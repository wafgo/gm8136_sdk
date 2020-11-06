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

#include "fmpeg4.h"
#include "./common/share_mem.h"
#include "./mpeg4_decoder/decoder.h"
#include "ioctl_mp4d.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMJPEG_D"
#include "debug.h"


extern unsigned int mp4_max_width;
extern unsigned int mp4_max_height;
/**********************************/
/* Gm mpeg4 decoder driver   */
/**********************************/
#define MAX_DEC_NUM     32
#define DEC_IDX_MASK_n(n)  (0x1<< n)
#define DEC_IDX_FULL      0xFFFFFFFF

static int dec_idx=0;

struct dec_private
{
	void *dec_handle;
	int Y_sz;
	int dev;
};

static struct dec_private dec_data[MAX_DEC_NUM] = {{0}};
extern unsigned int mp4_max_width;
extern unsigned int mp4_max_height;
//extern unsigned int mcp100_bs_virt, mcp100_bs_phy, mcp100_bs_length;
extern struct buffer_info_t mcp100_bs_buffer;
extern struct semaphore fmutex;
//unsigned int dec_raw_yuv_virt,dec_raw_yuv_phy;
struct buffer_info_t mp4_dec_raw_buffer = {0, 0, 0};

int fmpeg4_decoder_open(struct inode *inode, struct file *filp)
{
	int idx;
	struct dec_private * decp;

	F_DEBUG("mpeg4_decoder_open\n");
	if((dec_idx&DEC_IDX_FULL)==DEC_IDX_FULL)  {
		printk("Encoder Device Service Full,0x%x!\n",dec_idx);
		return -EFAULT;
	}
	down(&fmutex);
	for (idx = 0;idx < MAX_DEC_NUM; idx ++) {
		if ( (dec_idx&DEC_IDX_MASK_n(idx)) == 0) {
			dec_idx |= DEC_IDX_MASK_n(idx);
			break;
		}
	}
	up(&fmutex);	
	decp = (struct dec_private *)&dec_data[idx];
	memset(decp,0,sizeof(struct dec_private));
	decp->dev = idx;
	filp->private_data = decp;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		MOD_INC_USE_COUNT;
	#endif
	return 0; /* success */
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long fmpeg4_decoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int fmpeg4_decoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	unsigned int freesz;
	FMP4_DEC_FRAME tFrame;
	FMP4_DEC_DISPLAY tDisp;
	FMP4_DEC_RESULT tDecResult;
	FMP4_DEC_PARAM_MY tDecParam;
	FMP4_DEC_PARAM * ppub = (FMP4_DEC_PARAM *)&tDecParam.pub;
	int Reinit = 0;
	int ret=0;
	struct dec_private * decp = (struct dec_private *)filp->private_data;
	int dev = decp->dev;
    
	F_DEBUG("fmpeg4_decoder_ioctl:cmd=0x%x\n",cmd);
	if(((1<<dev)&dec_idx)==0)    {
		printk("No Support index %d for 0x%x\n",dev,dec_idx);
		return -EFAULT;
	}
	
	down(&fmutex);
	switch(cmd) {
		case FMPEG4_IOCTL_DECODE_REINIT:
			Reinit = 1;
			// no break here;
		case FMPEG4_IOCTL_DECODE_INIT:
			if (copy_from_user((void *)ppub, (void *)arg, sizeof(FMP4_DEC_PARAM))) {
					err("Error copy_from_user()");
					ret=-EFAULT;
					goto decoder_ioctl_exit;
			}
			if((ppub->u32API_version & 0xFFFFFFF0) != (MP4D_VER & 0xFFFFFFF0)) {
				printk("Fail API Version v%d.%d%d (Current Driver v%d.%d.%d)\n",
						ppub->u32API_version>>16,
						(ppub->u32API_version&0xffff) >> 4,
						(ppub->u32API_version&0x000f),
						MP4D_VER_MAJOR, MP4D_VER_MINOR, MP4D_VER_MINOR2);
				printk("Please upgrade your MPEG4 driver and re-compiler AP.\n");
				ret=-EFAULT;
				goto decoder_ioctl_exit;            
			}
			if ((ppub->u32MaxWidth == 0) || (ppub->u32MaxHeight  == 0)) {
				ppub->u32MaxWidth = mp4_max_width;
				ppub->u32MaxHeight = mp4_max_height;
			}
			if ((ppub->u32MaxWidth > mp4_max_width) || (ppub->u32MaxHeight > mp4_max_height)) {
				printk("input parameter over capacity (%dx%d > %dx%d)\n",
						ppub->u32MaxWidth, ppub->u32MaxHeight, mp4_max_width, mp4_max_height);
				ret=-EFAULT;
				goto decoder_ioctl_exit;
			}
			decp->Y_sz= ((ppub->u32FrameWidth + 15)/16) * 16 *
										((ppub->u32FrameHeight + 15)/16)*16;
			tDecParam.u32VirtualBaseAddr = (unsigned int)mcp100_dev->va_base;  
			//tDecParam.pu8BitStream=(unsigned char *)mcp100_bs_virt;
			//tDecParam.pu8BitStream_phy=(unsigned char *)mcp100_bs_phy;
			//tDecParam.u32BSBufSize = mcp100_bs_length;
			tDecParam.pu8BitStream=(unsigned char *)mcp100_bs_buffer.addr_va;
			tDecParam.pu8BitStream_phy=(unsigned char *)mcp100_bs_buffer.addr_pa;
			tDecParam.u32BSBufSize = mcp100_bs_buffer.size;            
			tDecParam.u32CacheAlign = 16;
			tDecParam.pfnMalloc = fkmalloc;
			tDecParam.pfnFree = fkfree;
			tDecParam.pfnDmaMalloc = fconsistent_alloc;
			tDecParam.pfnDmaFree = fconsistent_free;

			if (Reinit) {
				if (Mp4VDec_ReInit(&tDecParam, decp->dec_handle) < 0)
					ret = -EFAULT;
			}
			else 	if (decp->dec_handle) {
				printk("fmp4d: previous one %d was not released", decp->dev);
				ret = -EFAULT;
			}
			else {
				if (Mp4VDec_Init(&tDecParam, &decp->dec_handle) < 0)
					ret = -EFAULT;
			}
			break;
		case FMPEG4_IOCTL_DECODE_FRAME:
			if (copy_from_user((void *)&tFrame, (void *)arg, sizeof(tFrame))) {
					err("Error copy_from_user()");
					ret=-EFAULT;
					goto decoder_ioctl_exit;
			}
			//if (tFrame.bs_length > mcp100_bs_length) {
			if (tFrame.bs_length > mcp100_bs_buffer.size) {
				printk("fmp4d: bitstream size over limit, 0x%x > 0x%x\n", tFrame.bs_length, mcp100_bs_buffer.size);
				ret=-EFAULT;
				goto decoder_ioctl_exit;
			}
			if(decp->dec_handle == NULL) {
				printk("fmp4d: NULL dec_handle\n");
				ret=-EFAULT;
				goto decoder_ioctl_exit;
			}
			freesz = Mp4VDec_QueryEmptyBuffer(decp->dec_handle);
			if (freesz < tFrame.bs_length) {
				printk("fmp4d: not enough free bitstream buffer (0x%x < 0x%x)\n", mcp100_bs_buffer.size, tFrame.bs_length);
				ret=-EFAULT;
				goto decoder_ioctl_exit;
			}
			if(Mp4VDec_FillBuffer(decp->dec_handle,(unsigned char *)tFrame.bs_buf, tFrame.bs_length) < 0) {
				ret=-EFAULT;
				goto decoder_ioctl_exit;
			}
            #if (LINUX_VERSION_CODE == KERNEL_VERSION(3, 3, 0))
                    //__cpuc_flush_dcache_area((void *)mcp100_bs_virt, enc_frame.bitstream_size);
                    __cpuc_flush_dcache_area((void *)mcp100_bs_buffer.addr_va, tFrame.bs_length);
            
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
				//dmac_clean_range((void *)mcp100_bs_virt, (void *)(mcp100_bs_virt + tFrame.bs_length));
				//dma_consistent_sync((unsigned long)mcp100_bs_virt, (void __iomem *)mcp100_bs_phy, tFrame.bs_length, DMA_BIDIRECTIONAL);
				dma_consistent_sync((unsigned long)mcp100_bs_buffer.addr_va, (void __iomem *)mcp100_bs_buffer.addr_pa, tFrame.bs_length, DMA_TO_DEVICE);
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
				consistent_sync((void *)mcp100_bs_buffer.addr_va, tFrame.bs_length, DMA_TO_DEVICE);
			#endif

            /*
			Mp4VDec_SetOutputAddr(decp->dec_handle,(unsigned char *)user_va_to_pa(tFrame.output_va_y),
					(unsigned char *)user_va_to_pa(tFrame.output_va_u), (unsigned char *)user_va_to_pa(tFrame.output_va_v));
    		*/
    		Mp4VDec_SetOutputAddr(decp->dec_handle,(unsigned char *)mp4_dec_raw_buffer.addr_pa,
					(unsigned char *)(mp4_dec_raw_buffer.addr_pa + mp4_max_width * mp4_max_height),
					(unsigned char *)(mp4_dec_raw_buffer.addr_pa + mp4_max_width * mp4_max_height * 5 / 4));
			if(Mp4VDec_OneFrame(decp->dec_handle, &tDecResult) < 0)   {
				tFrame.got_picture=0;
				ret=-EFAULT;
			}
			else
				tFrame.got_picture=1;
			tFrame.bs_length=tDecResult.u32UsedBytes;
			if (copy_to_user((void *)arg,(void *)&tFrame,sizeof(tFrame))) {
				err("Error copy_to_user()");
				ret=-EFAULT;
			}
			break;
		case FMPEG4_IOCTL_DECODE_DISPLAY:
			if (copy_from_user((void *)&tDisp, (void *)arg, sizeof(tDisp))) {
					err("Error copy_from_user()");
					ret=-EFAULT;
					goto decoder_ioctl_exit;
			}
			if(decp->dec_handle == NULL) {
				printk("fmp4d: NULL dec_handle\n");
				ret=-EFAULT;
			}
			else
				Mp4VDec_SetCrop (decp->dec_handle, tDisp.crop_x, tDisp.crop_y, tDisp.crop_w, tDisp.crop_h);
			break;
		default:
			printk("Not support this IOCTL 0x%x \n", cmd);
			ret = -EFAULT;
			break;
	}

decoder_ioctl_exit:
	up(&fmutex);
	return ret;
}


//get continue output Y,U,V User address

int fmpeg4_decoder_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if 0
	unsigned int size = vma->vm_end - vma->vm_start;
	unsigned int yuv_size= DIV_M10000(mp4_max_width * mp4_max_height * 2);	// for yuv422

	//printk("Alloc va 0x%x size 0x%x\n",vma->vm_start,size);
	if(size > yuv_size) {
		printk("The reauest YUV size 0x%x must less than 0x%x bytes\n", size, yuv_size);
		return -EFAULT;
	}

	if(0 == dec_raw_yuv_virt) {
		dec_raw_yuv_virt = (unsigned int)fcnsist_alloc(PAGE_ALIGN(yuv_size),
			(void **)&dec_raw_yuv_phy, 0/*ncnb*/);
		//printk ("alloc Y va=0x%x pa=0x%x\n", dec_raw_yuv_virt, dec_raw_yuv_phy);
		if(0 == dec_raw_yuv_virt) {
			printk("Fail to allocate decode raw buffer!\n");
			return -EFAULT;
		}
	}
	vma->vm_pgoff = dec_raw_yuv_phy >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		fcnsist_free (PAGE_ALIGN(yuv_size), (void *)dec_raw_yuv_virt, (void *)dec_raw_yuv_phy);
		return -EAGAIN;
	}
#else
    unsigned int size = vma->vm_end - vma->vm_start;

    vma->vm_pgoff = mp4_dec_raw_buffer.addr_pa >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		//fcnsist_free (PAGE_ALIGN(yuv_size), (void *)dec_raw_yuv_virt, (void *)dec_raw_yuv_phy);
		return -EAGAIN;
	}
#endif
	return 0;
}

int fmpeg4_decoder_release(struct inode *inode, struct file *filp)
{
	struct dec_private * decp = (struct dec_private *)filp->private_data;
	int dev = decp->dev;

	down(&fmutex);
	if(decp->dec_handle)
		Mp4VDec_Release(decp->dec_handle);
	filp->private_data=0;
	decp->dec_handle=0;
	dec_idx &= (~(DEC_IDX_MASK_n(dev)) );	
	up(&fmutex);

	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
static struct file_operations fmpeg4_decoder_fops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    .unlocked_ioctl = fmpeg4_decoder_ioctl,
#else
	.ioctl = fmpeg4_decoder_ioctl,
#endif
	.mmap = fmpeg4_decoder_mmap,
	.open = fmpeg4_decoder_open,
	.release = fmpeg4_decoder_release,
};
	
static struct miscdevice fmpeg4_decoder_dev = {
	.minor = FMPEG4_DECODER_MINOR,
	.name = "fdec",
	.fops = &fmpeg4_decoder_fops,
};

#else
static struct file_operations fmpeg4_decoder_fops = {
	.owner =      THIS_MODULE,
	.ioctl =		fmpeg4_decoder_ioctl,
	.mmap =       fmpeg4_decoder_mmap,
	.open =		fmpeg4_decoder_open,
	.release =	fmpeg4_decoder_release,
};
	
static struct miscdevice fmpeg4_decoder_dev = {
	.minor = FMPEG4_DECODER_MINOR,
	.name = "fmpeg4 decoder",
	.fops = &fmpeg4_decoder_fops,
};
#endif

static void mp4d_isr(int irq, void *dev_id, unsigned int base)
{
	MP4_COMMAND type;
	volatile Share_Node *node = (volatile Share_Node *) (0x8000 + base);		 

	type = (node->command & 0xF0)>>4;
	if(( type == DECODE_IFRAME) || (type == DECODE_PFRAME) || (type == DECODE_NFRAME) ) {
		#ifdef EVALUATION_PERFORMANCE
			encframe.hw_stop = get_counter();
		#endif
		#ifndef GM8120
			*(volatile unsigned int *)(base+0x20098)=0x80000000;
		#endif
	}
}

extern void switch2mp4d(void * codec, ACTIVE_TYPE curr);
static mcp100_rdev mp4ddv = {
	.handler = mp4d_isr,
	.dev_id = NULL,
	.switch22 = switch2mp4d,
};

//int drv_mp4d_init(void)
int mp4d_drv_init(void)
{
	if (mcp100_register(&mp4ddv, TYPE_MPEG4_DECODER, MP4D_MAX_CHANNEL) < 0)
		return -1;
	if(misc_register(&fmpeg4_decoder_dev) < 0){
		err("fmpeg4_decoder_dev misc-register fail");
		mcp100_deregister (TYPE_MPEG4_DECODER);
		return -1;
	}
    if (allocate_buffer(&mp4_dec_raw_buffer, mp4_max_width * mp4_max_height * 2) < 0) {
        err("fmpeg4 decoder allocate raw data error\n");
        return -1;
    }
	return 0;
} 

//void drv_mp4d_exit(void)
void mp4d_drv_close(void)
{
/*
	unsigned int yuv_size= DIV_M10000(mp4_max_width * mp4_max_height * 2);
	if (dec_raw_yuv_virt)
		fcnsist_free (PAGE_ALIGN(yuv_size), (void *)dec_raw_yuv_virt, (void *)dec_raw_yuv_phy);
*/
    free_buffer(&mp4_dec_raw_buffer);
	mcp100_deregister (TYPE_MPEG4_DECODER);
	misc_deregister(&fmpeg4_decoder_dev);
}
