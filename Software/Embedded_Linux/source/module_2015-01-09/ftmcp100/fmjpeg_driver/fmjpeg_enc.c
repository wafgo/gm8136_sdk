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
#include "encoder/jpeg_enc.h"
#include "ioctl_je.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMJPEG_E"
#include "debug.h"

#define CACHE_LINE 16
#if 1
#define F_DEBUG(fmt, args...) printk("FMJPEG: " fmt, ## args)
#else
#define F_DEBUG(a...)
#endif

#define MAX_JENC_NUM      MJE_MAX_CHANNEL
#define JENC_IDX_MASK_n(n)  (0x1<<n)
#define JENC_IDX_FULL    0xFFFFFFFF

static int jenc_idx = 0;

struct jenc_private
{
	void * enc_handle;
	int Y_sz;
	int dev;
};

static struct jenc_private jenc_data[MAX_JENC_NUM] = {{0}};
extern unsigned int jpg_max_width;
extern unsigned int jpg_max_height;
//extern unsigned int mcp100_bs_virt, mcp100_bs_phy, mcp100_bs_length;
extern struct buffer_info_t mcp100_bs_buffer;
//unsigned int mjpeg_enc_raw_yuv_virt=0, mjpeg_enc_raw_yuv_phy=0;
extern struct semaphore fmutex;
static struct buffer_info_t mjpeg_enc_raw_buffer = {0, 0, 0};

int fmjpeg_encoder_open(struct inode *inode, struct file *filp)
{
	int idx;
	struct jenc_private * encp;

	if((jenc_idx&JENC_IDX_FULL)==JENC_IDX_FULL) {
		F_DEBUG("Encoder Device Service Full,0x%x!\n",jenc_idx);
		return -EFAULT;
	}

	down(&fmutex);
	for (idx = 0; idx < MAX_JENC_NUM; idx ++) {
		if( (jenc_idx&JENC_IDX_MASK_n(idx)) == 0) {
			jenc_idx |= JENC_IDX_MASK_n(idx);
			break;
		}
	}
	up(&fmutex);
	encp = (struct jenc_private *)&jenc_data[idx];
	memset(encp,0,sizeof(struct jenc_private));
	encp->dev = idx;
	filp->private_data = encp;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		MOD_INC_USE_COUNT;
	#endif      		
	return 0;
}

static int fmje_create(struct jenc_private * pv, FJPEG_ENC_PARAM * encm, int reinit)
{
	FJPEG_ENC_PARAM_MY enc_param;
	memcpy ((void *)&enc_param.pub, (void *)encm, sizeof(FJPEG_ENC_PARAM));	

	if ((enc_param.pub.u32API_version & 0xFFFFFF00) != (MJPEGE_VER & 0xFFFFFF00)) {
		printk("Fail API Version v%d.%d.%d (Current Driver v%d.%d.%d)\n",
			(enc_param.pub.u32API_version>>28)&0x0F,
			(enc_param.pub.u32API_version>>20)&0xFF,
			(enc_param.pub.u32API_version>>8)&0x0FFF,
			MJPEGE_VER_MAJOR, MJPEGE_VER_MINOR, MJPEGE_VER_MINOR2);
		printk("Please upgrade your JPEG driver and re-compiler AP.\n");
		return -EFAULT;
	}
	enc_param.pfnDmaFree = fconsistent_free;
	enc_param.pfnDmaMalloc = fconsistent_alloc;
	enc_param.pfnMalloc = fkmalloc;
	enc_param.pfnFree = fkfree;
	enc_param.u32CacheAlign = CACHE_LINE;
	enc_param.pu32BaseAddr = (unsigned int*)mcp100_dev->va_base;  
    /*
	enc_param.bs_buffer_virt=(unsigned char *)mcp100_bs_virt;
	enc_param.bs_buffer_phy=(unsigned char *)mcp100_bs_phy;
	*/
    enc_param.bs_buffer_virt=(unsigned char *)mcp100_bs_buffer.addr_va;
    enc_param.bs_buffer_phy=(unsigned char *)mcp100_bs_buffer.addr_pa;

	// to create the jpeg encoder object
	if (reinit) {
		if (FJpegEncCreate_fake(&enc_param, pv->enc_handle) < 0) {
			printk("fmjpeg_enc not had create\n");	
			return -EFAULT;
		}
	}
	else	if (pv->enc_handle) {
		printk("fmje: previous one %d was not released", pv->dev);
		return -EFAULT;
	}
	else {
		pv->enc_handle =FJpegEncCreate(&enc_param);
		if (!pv->enc_handle  ) {
			printk("fmjpeg_enc not had create\n");	
			return -EFAULT;
		}
	}
	pv->Y_sz = enc_param.pub.u32ImageHeight * enc_param.pub.u32ImageWidth;
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long fmjpeg_encoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int fmjpeg_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	FJPEG_ENC_PARAM encp;
	FJPEG_ENC_FRAME enc_frame;
	FJPEG_ENC_QTBLS enc_qtbls;
    void * user_va;
    int ret=0, i;
	struct jenc_private * encpv = (struct jenc_private * )filp->private_data;
	int dev = encpv->dev;
	int reinit = 0;
	 	
	if(((1<<dev)&jenc_idx)==0) {
		F_DEBUG("No Support index %d for 0x%x\n",dev,jenc_idx);
		return -EFAULT;
	}	

	down(&fmutex);
	switch(cmd) {
		case FMJPEG_IOCTL_ENCODE_RECREATE:
			reinit = 1;
			// no break here
		case FMJPEG_IOCTL_ENCODE_CREATE:
			if (copy_from_user((void *)&encp, (void *)arg, sizeof(FJPEG_ENC_PARAM))) {
				err("Err copy_from_user");
				ret = -EFAULT;
			}
			else
				ret = fmje_create(encpv, &encp, reinit);
			break;
		case FMJPEG_IOCTL_ENCODE_ONE:
			if (copy_from_user((void *)&enc_frame, (void *)arg, sizeof(enc_frame))) {
 				err("Err copy_from_user");
				ret = -EFAULT;
				break;
			}
			if ( !encpv->enc_handle ) {
				printk("fmjpeg_enc not had create\n");	
				ret=-EFAULT;
				break;
 			}	
			user_va=enc_frame.pu8BitstreamAddr;  	// user buffer virt-addr
			/*
			if(enc_frame.pu8YUVAddr[0]!=0) {
				if(!check_continued((unsigned int)enc_frame.pu8YUVAddr[0], encpv->Y_sz)) {
					F_DEBUG("Warring! The encode Y input buffer not continued! Please use mmap instead.\n");
					ret=-EFAULT;
					goto encoder_ioctl_exit;
				}
			}
			for(i=0;i<3;i++)
				enc_frame.pu8YUVAddr[i] = (unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8YUVAddr[i]);
	        */
	        for(i=0;i<3;i++)
				enc_frame.pu8YUVAddr[i] = (unsigned char *)mjpeg_enc_raw_buffer.addr_pa + jpg_max_width * jpg_max_height * i;
			//enc_frame.pu8BitstreamAddr =(unsigned char *)mcp100_bs_phy;  //change to PA
			//enc_frame.bitstream_size = mcp100_bs_length;
			enc_frame.pu8BitstreamAddr =(unsigned char *)mcp100_bs_buffer.addr_pa;  //change to PA
			enc_frame.bitstream_size = mcp100_bs_buffer.size;
			enc_frame.bitstream_size = FJpegEncEncode(encpv->enc_handle, &enc_frame);  
			if (enc_frame.bitstream_size == 0)
				ret=-EFAULT;
			else {
                #if (LINUX_VERSION_CODE == KERNEL_VERSION(3, 3, 0))
                    //__cpuc_flush_dcache_area((void *)mcp100_bs_virt, enc_frame.bitstream_size);
                    __cpuc_flush_dcache_area((void *)mcp100_bs_buffer.addr_va, enc_frame.bitstream_size);
				#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
					//dma_consistent_sync((unsigned long)mcp100_bs_virt, (void __iomem *)mcp100_bs_phy, enc_frame.bitstream_size, DMA_FROM_DEVICE);
				#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
					//consistent_sync((void *)mcp100_bs_virt, enc_frame.bitstream_size, DMA_FROM_DEVICE);
				#endif
				//copy_to_user((void *)user_va,(void *)enc_param.bs_buffer_virt, enc_param.bitstream_size); // return buffer virtual address			
				//if (copy_to_user((void *)user_va,(void *)mcp100_bs_virt, enc_frame.bitstream_size)) {
				if (copy_to_user((void *)user_va,(void *)mcp100_bs_buffer.addr_va, enc_frame.bitstream_size)) {
	 				err("Err copy_to_user");
					ret = -EFAULT;
					break;
				}
			}
			if (copy_to_user((void *)arg, (void *)&enc_frame, sizeof(enc_frame))) {
 				err("Err copy_to_user");
				ret = -EFAULT;
			}
			break;
		case FMJPEG_IOCTL_ENCODE_QUANTTBL:
			if (copy_from_user((void *)&enc_qtbls, (void *)arg, sizeof(FJPEG_ENC_QTBLS)) ) {
	 			err("Err copy_from_user");
				ret = -EFAULT;
			}
			else if ( !encpv->enc_handle ) {
				printk("fmjpeg_enc not had create\n");	
				ret = -EFAULT;
			}
			else {
				if (FJpegEncSetQTbls(encpv->enc_handle, &enc_qtbls.luma_qtbl[0], &enc_qtbls.chroma_qtbl[0]) < 0)
					ret = -EFAULT;
			}
			break;
		case FMJPEG_IOCTL_ENCODE_CURR_QTBL:
			if ( !encpv->enc_handle ) {
				printk("fmjpeg_enc not had create\n");	
				ret=-EFAULT;
			}
			else {
				FJpegEncGetQTbls(encpv->enc_handle, &enc_qtbls);
				if (copy_to_user((void *)arg,(void *)&enc_qtbls,sizeof(FJPEG_ENC_QTBLS)) ) {
	 				err("Err copy_to_user");
					ret = -EFAULT;
				}
			}
			break;
		default:
			printk("mjpeg_d: not support this ioctl_no 0x%x\n", cmd);
			ret=-EFAULT;
			break;
	}
encoder_ioctl_exit:
    up(&fmutex);
    return ret;
}

#define TIMES 3
//get continue input Y,U,V User address
int fmjpeg_encoder_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	unsigned int size = vma->vm_end - vma->vm_start;  
	unsigned int yuv_size ;

	if((jpg_max_width > 720) && (jpg_max_height > 576) )
		yuv_size = DIV_J10000(jpg_max_width * jpg_max_height * TIMES);	
	else
		yuv_size = DIV_J1000(jpg_max_width * jpg_max_height * TIMES);

    
	//printk("Alloc va 0x%x size 0x%x\n",vma->vm_start,size);
	if(size > yuv_size) {
		printk("The request YUV size 0x%x must less than 0x%x bytes\n", size, yuv_size);
		return -EFAULT;
	}

	if (0 == mjpeg_enc_raw_yuv_virt) {
		mjpeg_enc_raw_yuv_virt = (unsigned int)fcnsist_alloc(PAGE_ALIGN(yuv_size),
			(void **)&mjpeg_enc_raw_yuv_phy, 0/*ncnb*/);
		if(0 == mjpeg_enc_raw_yuv_virt) {
			printk("Fail to allocate encode raw buffer!\n");
			return -EFAULT;
		}
	}
	vma->vm_pgoff = mjpeg_enc_raw_yuv_phy >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_enc_raw_yuv_virt, (void *) mjpeg_enc_raw_yuv_phy);
		return -EAGAIN;
	}
#else
    unsigned int size = vma->vm_end - vma->vm_start;  

	vma->vm_pgoff = mjpeg_enc_raw_buffer.addr_pa >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		//fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_enc_raw_yuv_virt, (void *) mjpeg_enc_raw_yuv_phy);
		return -EAGAIN;
	}

#endif
	return 0;
}


int fmjpeg_encoder_release(struct inode *inode, struct file *filp)
{
	struct jenc_private * encpv = (struct jenc_private * )filp->private_data;
	int dev = encpv->dev;

	down(&fmutex);
	if( encpv->enc_handle )
		FJpegEncDestroy(encpv->enc_handle);
	filp->private_data = 0;   
	encpv->enc_handle = 0;
	jenc_idx &= ~(JENC_IDX_MASK_n(dev));
	up(&fmutex);
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
static struct file_operations fmjpeg_encoder_fops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    .unlocked_ioctl = fmjpeg_encoder_ioctl,
#else
	.ioctl = fmjpeg_encoder_ioctl,
#endif
	.mmap = fmjpeg_encoder_mmap,
	.open = fmjpeg_encoder_open,
	.release = fmjpeg_encoder_release,
};

static struct miscdevice fmjpeg_encoder_dev = {
	.minor = FMJPEG_ENCODER_MINOR,
	.name = "mjenc",
	.fops = &fmjpeg_encoder_fops,
};
#else
static struct file_operations fmjpeg_encoder_fops = {
	.owner =     THIS_MODULE,
	.ioctl =		fmjpeg_encoder_ioctl,
	.mmap =     	fmjpeg_encoder_mmap,
	.open = 	fmjpeg_encoder_open,
	.release =	fmjpeg_encoder_release,
};

static struct miscdevice fmjpeg_encoder_dev = {
	.minor = FMJPEG_ENCODER_MINOR,
	.name = "fmjpeg encoder",
	.fops = &fmjpeg_encoder_fops,
};
#endif

int je_int;
static void mje_isr(int irq, void *dev_id, unsigned int base)
{
	if (je_int & IT_JPGE) {
		unsigned int int_sts;
		int_sts = mSEQ_INTFLG(base);
		// clr interrupt
		mSEQ_INTFLG(base) = int_sts;
		je_int = int_sts;
	}
}
static mcp100_rdev mjedv = {
	.handler = mje_isr,
	.dev_id = NULL,
	.switch22 = NULL,
};

//int drv_mje_init(void)
int mje_drv_init(void)
{
	if (mcp100_register(&mjedv, TYPE_JPEG_ENCODER, jpg_enc_max_chn) < 0)
		return -1;
	if(misc_register(&fmjpeg_encoder_dev) < 0) {
		err("can't register fmjpeg_encoder_dev");
		mcp100_deregister (TYPE_JPEG_ENCODER);
		return -1;
	}
    if (allocate_buffer(&mjpeg_enc_raw_buffer, jpg_max_width * jpg_max_height * 3) < 0) {
        err("allocate jpeg raw data error\n");
        return -1;
    }
	return 0;
} 

//void drv_mje_exit(void)
void mje_drv_close(void)
{
/*
	int yuv_size;

	if((jpg_max_width > 720) && (jpg_max_height > 576))
		yuv_size = DIV_J10000(jpg_max_width * jpg_max_height * TIMES);	
	else
		yuv_size = DIV_J1000(jpg_max_width * jpg_max_height * TIMES);

	if(mjpeg_enc_raw_yuv_virt)
		fcnsist_free(PAGE_ALIGN(yuv_size), (void *)mjpeg_enc_raw_yuv_virt, (void *)mjpeg_enc_raw_yuv_phy);
*/
    free_buffer(&mjpeg_enc_raw_buffer);
	mcp100_deregister (TYPE_JPEG_ENCODER);
	misc_deregister(&fmjpeg_encoder_dev);
}
