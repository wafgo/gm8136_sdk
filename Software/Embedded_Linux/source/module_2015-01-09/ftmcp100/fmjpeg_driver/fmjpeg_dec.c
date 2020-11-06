#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>


#include "fmjpeg.h"
#include "decoder/jpeg_dec.h"
#include "ioctl_jd.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMJPEG_D"
#include "debug.h"


#ifndef CACHE_LINE 
#define CACHE_LINE 16
#endif

/**********************************/
/* Gm mjpeg decoder driver   */
/**********************************/
#define MAX_JDEC_NUM     MJD_MAX_CHANNEL
#define JDEC_IDX_MASK_n(n) (0x1<<n)
#define JDEC_IDX_FULL    0xFFFFFFFF

static int jdec_idx = 0;
struct jdec_private
{
	void * dec_handle;
	int frame_width;
	int frame_height;
	int dev;
};

static struct jdec_private jdec_data[MAX_JDEC_NUM] = {{0}};
extern unsigned int jpg_max_width;
extern unsigned int jpg_max_height;
//extern unsigned int mcp100_bs_virt, mcp100_bs_phy, mcp100_bs_length;
extern struct buffer_info_t mcp100_bs_buffer;
//unsigned int mjpeg_dec_raw_yuv_virt=0, mjpeg_dec_raw_yuv_phy=0;
extern struct semaphore fmutex;
static struct buffer_info_t mjpeg_dec_raw_buffer = {0, 0, 0};

int fmjpeg_decoder_open(struct inode *inode, struct file *filp)
{
	int idx;
	struct jdec_private * decp;

	//F_DEBUG("mjpeg_decoder_open\n");

	if((jdec_idx & JDEC_IDX_FULL) == JDEC_IDX_FULL)  {
		F_DEBUG("Decoder Device Service Full,0x%x!\n",jdec_idx);
		return -EFAULT;
	}
	down(&fmutex);
	for (idx = 0; idx < MAX_JDEC_NUM; idx ++) {
		if( (jdec_idx&JDEC_IDX_MASK_n(idx)) == 0) {
			jdec_idx |= JDEC_IDX_MASK_n(idx);
			break;
		}
	}
	up(&fmutex);
	decp = (struct jdec_private *)&jdec_data[idx];
	memset(decp,0,sizeof(struct jdec_private));
	decp->dev = idx;
	filp->private_data = decp;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		MOD_INC_USE_COUNT;
	#endif
	return 0; /* success */
}
void jd_copy_from_user(void* dst, void * src, int byte_sz)
{
	if (copy_from_user(dst, src, byte_sz))
		err("Error copy_from_user()");
}
void jd_sync2device(void* v_ptr, void *p_ptr, int byte_sz)
{
    #if (LINUX_VERSION_CODE == KERNEL_VERSION(3, 3, 0))
        __cpuc_flush_dcache_area(v_ptr, byte_sz);
    #elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
		dma_consistent_sync((unsigned long)p_ptr, (void __iomem *)v_ptr, byte_sz, DMA_TO_DEVICE);
	#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		consistent_sync(v_ptr, byte_sz, DMA_TO_DEVICE);
	#endif
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long fmjpeg_decoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int fmjpeg_decoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	FJPEG_DEC_PARAM_MY tDecParam;
	FJPEG_DEC_FRAME tDecFrame;
	FJPEG_DEC_PARAM_A2 tPA2;
	int ret=-EFAULT;
	int passhuf = 1;
	struct jdec_private * decp = (struct jdec_private *)filp->private_data;
	int dev = decp->dev;
	int reinit = 0;

	if(((1<<dev)&jdec_idx)==0)  {
		printk("No Support index %d for 0x%x\n",dev,jdec_idx);
		return -EFAULT;
	}
	//F_DEBUG("fmjpeg_decoder_ioctl:cmd=0x%x\n",cmd);
	down(&fmutex);
	switch(cmd) {
		case FMJPEG_IOCTL_DECODE_RECREATE:
			reinit = 1;
			// no break here
		case FMJPEG_IOCTL_DECODE_CREATE:
			if (copy_from_user((void *)&tDecParam.pub, (void *)arg, sizeof(FJPEG_DEC_PARAM))) {
					err("Error copy_from_user()");
					goto decoder_ioctl_exit;
			}
			if((tDecParam.pub.u32API_version&0xFFFFFFF0) != (MJPEGD_VER&0xFFFFFFF0)) {
				printk("Fail API Version v%d.%d%d (Current Driver v%d.%d.%d)\n",
					tDecParam.pub.u32API_version>>16,
					(tDecParam.pub.u32API_version&0xffff)>>4,
					(tDecParam.pub.u32API_version&0x000f),
					MJPEGD_VER_MAJOR, MJPEGD_VER_MINOR, MJPEGD_VER_MINOR2);
				goto decoder_ioctl_exit;
			}

			tDecParam.pu32BaseAddr = (unsigned int*) mcp100_dev->va_base; 
 			tDecParam.pfnMalloc = fkmalloc;
			tDecParam.pfnFree = fkfree;
			tDecParam.u32CacheAlign = CACHE_LINE;
            /*
			tDecParam.bs_buffer_phy = mcp100_bs_phy;
			tDecParam.bs_buffer_virt = (unsigned char *)mcp100_bs_virt;
			tDecParam.bs_buffer_len = mcp100_bs_length;
			*/
            tDecParam.bs_buffer_phy = mcp100_bs_buffer.addr_pa;
            tDecParam.bs_buffer_virt = (unsigned char *)mcp100_bs_buffer.addr_va;
            tDecParam.bs_buffer_len = mcp100_bs_buffer.size;

			// to create the jpeg decoder object
			if (reinit)
				FJpegDecReCreate(&tDecParam, decp->dec_handle);
			else if (decp->dec_handle) {
				err("previous one %d was not released", decp->dev);
				goto decoder_ioctl_exit;
			}
			else {
				decp->dec_handle = FJpegDecCreate(&tDecParam);
				if(decp->dec_handle == NULL)
					goto decoder_ioctl_exit;
			}
			break;
		case FMJPEG_IOCTL_DECODE_CREATE_PA2:
			if (copy_from_user((void *)&tPA2, (void *)arg, sizeof(FJPEG_DEC_PARAM_A2))) {
					err("Error copy_from_user()");
					goto decoder_ioctl_exit;
			}
			if ( !decp->dec_handle) {
				printk("fmjpeg_dec had not been created\n");	
				goto decoder_ioctl_exit;
			}
			FJpegDecParaA2(decp->dec_handle, &tPA2);
			break;
		case FMJPEG_IOCTL_DECODE_ONE:
			passhuf = 0;
			// no break here;
		case FMJPEG_IOCTL_DECODE_PASSHUF:
			if (copy_from_user((void *)&tDecFrame, (void *)arg, sizeof(FJPEG_DEC_FRAME))) {
					err("Error copy_from_user()");
					goto decoder_ioctl_exit;
			}
			if ( !decp->dec_handle) {
				printk("fmjpeg_dec had not been created\n");	
				goto decoder_ioctl_exit;
			}
			FJpegDec_fill_bs(decp->dec_handle, tDecFrame.buf, tDecFrame.buf_size);
			if (FJpegDecReadHeader(decp->dec_handle,&tDecFrame, passhuf) < 0) {
				if (copy_to_user((void *)arg, (void *)&tDecFrame, sizeof(FJPEG_DEC_FRAME)))
					err("Error copy_to_user()");
				goto decoder_ioctl_exit;
			}
			if (tDecFrame.pu8YUVAddr[0] != NULL) {
                #if 0
				tDecFrame.pu8YUVAddr[0] = (unsigned char *)user_va_to_pa((unsigned int)tDecFrame.pu8YUVAddr[0] );
				tDecFrame.pu8YUVAddr[1] = (unsigned char *)user_va_to_pa((unsigned int)tDecFrame.pu8YUVAddr[1] );
				tDecFrame.pu8YUVAddr[2] = (unsigned char *)user_va_to_pa((unsigned int)tDecFrame.pu8YUVAddr[2] );
                #else
				tDecFrame.pu8YUVAddr[0] = (unsigned char *)mjpeg_dec_raw_buffer.addr_pa;
				tDecFrame.pu8YUVAddr[1] = (unsigned char *)mjpeg_dec_raw_buffer.addr_pa + jpg_max_width * jpg_max_height;
				tDecFrame.pu8YUVAddr[2] = (unsigned char *)mjpeg_dec_raw_buffer.addr_pa + jpg_max_width * jpg_max_height * 2;
                #endif
				if (FJpegDecDecode(decp->dec_handle, &tDecFrame) < 0) {
					if (copy_to_user((void *)arg, (void *)&tDecFrame, sizeof(FJPEG_DEC_FRAME)))
						err("Error copy_to_user()");
					goto decoder_ioctl_exit;
				}
			}
			else {
				extern void stop_jd(void *dec_handle);
				stop_jd(decp->dec_handle);
			}
			if (copy_to_user((void *)arg, (void *)&tDecFrame, sizeof(FJPEG_DEC_FRAME))) {
				err("Error copy_to_user()");
				goto decoder_ioctl_exit;
			}
			break;
#ifndef SEQ
		case FMJPEG_IOCTL_DECODE_DISPSET:
			{
				FJPEG_DEC_DISPLAY tDecDisp;
				if (copy_from_user((void *)&tDecDisp, (void *)arg, sizeof(FJPEG_DEC_DISPLAY)) ) {
					err("Error copy_from_user()");
					goto decoder_ioctl_exit;
				}
				if ( !decp->dec_handle) {
					printk("fmjpeg_de not had create\n");	
					goto decoder_ioctl_exit;
				}
				FJpegDecSetDisp (decp->dec_handle, &tDecDisp);
			}
			break;
#endif
		default:
			printk("mjpeg_d: not support this ioctl_no 0x%x\n", cmd);
			goto decoder_ioctl_exit;
	}
	ret = 0;	// success

decoder_ioctl_exit:
	up(&fmutex);
	return ret;
}

#define TIMES 3
int fmjpeg_decoder_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	unsigned int size = vma->vm_end - vma->vm_start;
	unsigned int yuv_size;
	
	if((jpg_max_width > 720) && (jpg_max_height> 576))
		yuv_size = DIV_J10000(jpg_max_width * jpg_max_height * TIMES);	
	else
		yuv_size = DIV_J1000(jpg_max_width * jpg_max_height * TIMES);	

	if(size > yuv_size)    {
		printk("The YUV size (%d) must less than %d bytes\n", size, yuv_size);
		return -EFAULT;
	}

	if (0 == mjpeg_dec_raw_yuv_virt) {
		mjpeg_dec_raw_yuv_virt = (unsigned int)fcnsist_alloc(PAGE_ALIGN(yuv_size),
				(void **)&mjpeg_dec_raw_yuv_phy, 0);	// ncnb
		if (0 == mjpeg_dec_raw_yuv_virt) {
			printk("Fail to allocate decode raw buffer size 0x%x!\n", (int)PAGE_ALIGN(yuv_size));
			return -EFAULT;
		}
	}
	vma->vm_pgoff = mjpeg_dec_raw_yuv_phy >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_dec_raw_yuv_virt, (void *)mjpeg_dec_raw_yuv_phy);
		return -EAGAIN;
	}
#else
    unsigned int size = vma->vm_end - vma->vm_start;
	
	vma->vm_pgoff = mjpeg_dec_raw_buffer.addr_pa >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		//fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_dec_raw_yuv_virt, (void *)mjpeg_dec_raw_yuv_phy);
		return -EAGAIN;
	}

#endif
	return 0;
}

int fmjpeg_decoder_release(struct inode *inode, struct file *filp)
{
	struct jdec_private * decp = (struct jdec_private *)filp->private_data;
	int dev = decp->dev;

	down(&fmutex);
	if( decp->dec_handle)
		FJpegDecDestroy(decp->dec_handle);
	filp->private_data = 0;
	decp->dec_handle = NULL;
	jdec_idx&=(~(JDEC_IDX_MASK_n(dev)));
	up(&fmutex);
	return 0;
}


static struct file_operations fmjpeg_decoder_fops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    .unlocked_ioctl = fmjpeg_decoder_ioctl,
#else
	.ioctl = fmjpeg_decoder_ioctl,
#endif
	.mmap = fmjpeg_decoder_mmap,
	.open = fmjpeg_decoder_open,
	.release = fmjpeg_decoder_release,
};

static struct miscdevice fmjpeg_decoder_dev = {
	.minor= FMJPEG_DECODER_MINOR,
	.name= "mjdec",
	.fops= &fmjpeg_decoder_fops,
};

int jd_int;
static void mjd_isr(int irq, void *dev_id, unsigned int base)
{
	if (jd_int & IT_JPGD) {
		unsigned int int_sts;
		int_sts = mSEQ_INTFLG(base);
		// clr interrupt
		mSEQ_INTFLG(base) = int_sts;
		jd_int = int_sts;
	}
}
static mcp100_rdev mjddv = {
	.handler = mjd_isr,
	.dev_id = NULL,
	.switch22 = NULL,
};
//int drv_mjd_init(void)
int mjd_drv_init(void)
{
	if (mcp100_register(&mjddv, TYPE_JPEG_DECODER, jpg_dec_max_chn) < 0)
		return -1;
	if(misc_register(&fmjpeg_decoder_dev) < 0) {
		err("can't register fmjpeg_encoder_dev");
		mcp100_deregister (TYPE_JPEG_DECODER);
		return -1;
	}
    if (allocate_buffer(&mjpeg_dec_raw_buffer, jpg_max_width * jpg_max_height * 3) < 0) {
        err("allocate jpeg dec raw data error\n");
        return -1;
    }
	return 0;
} 

//void drv_mjd_exit(void)
void mjd_drv_close(void)
{
/*
	int yuv_size;

	if((jpg_max_width > 720) && (jpg_max_height > 576))
		yuv_size= DIV_J10000(jpg_max_width * jpg_max_height * TIMES);	
	else
		yuv_size= DIV_J1000(jpg_max_width * jpg_max_height * TIMES);	
	if (mjpeg_dec_raw_yuv_virt)
		fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_dec_raw_yuv_virt, (void *)mjpeg_dec_raw_yuv_phy);
*/
    free_buffer(&mjpeg_dec_raw_buffer);
	mcp100_deregister (TYPE_JPEG_DECODER);
 	misc_deregister(&fmjpeg_decoder_dev);
}

