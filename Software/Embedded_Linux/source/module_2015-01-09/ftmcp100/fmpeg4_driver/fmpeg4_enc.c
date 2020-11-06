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
#include "./mpeg4_encoder/encoder.h"
#include "ioctl_mp4e.h"
#include "../fmcp.h"
#undef  PFX
#define PFX	        "FMPEG4_E"
#include "debug.h"

#define MAX_ENC_NUM     MP4E_MAX_CHANNEL
#define ENC_IDX_MASK_n(n)  (0x1 << n)
#define ENC_IDX_FULL    0xFFFFFFFF
#define QUANT_SIZE	64

extern struct semaphore fmutex;
extern unsigned int mp4_max_width;
extern unsigned int mp4_max_height;
//extern unsigned int mcp100_bs_virt, mcp100_bs_phy, mcp100_bs_length;
//unsigned int enc_raw_yuv_virt = 0,enc_raw_yuv_phy = 0;
extern struct buffer_info_t mcp100_bs_buffer;
struct buffer_info_t mp4_enc_raw_buffer = {0, 0, 0};

static int enc_idx= 0;
int FMpeg4EncFrameCnt(void *enc_handle);

struct enc_private
{
	void * enc_handle;
	int Y_sz;
	unsigned char * customer_inter_quant;
	unsigned char * customer_intra_quant;
	int dev;
};
static struct enc_private enc_data[MAX_ENC_NUM] = {{0}};

int fmpeg4_encoder_open(struct inode *inode, struct file *filp)
{
	int idx;
	struct enc_private * encp;

	F_DEBUG("mpeg4_encoder_open\n");
	if((enc_idx&ENC_IDX_FULL)==ENC_IDX_FULL)     {
		printk("Encoder Device Service Full,0x%x!\n",enc_idx);
		return -EFAULT;
	}

	down(&fmutex);
	for (idx = 0; idx < MAX_ENC_NUM; idx ++) {
		if( (enc_idx&ENC_IDX_MASK_n(idx)) == 0 ) {
			enc_idx |= ENC_IDX_MASK_n(idx);	
			break;
		}
	}
	up(&fmutex);
	encp = &enc_data[idx];
	memset(encp,0,sizeof(struct enc_private));
	encp->dev = idx;
	//printk("idx=%d enc_idx=0x%x\n",idx,enc_idx);
	filp->private_data = encp;
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
		MOD_INC_USE_COUNT;
	#endif
	return 0; /* success */
}

static int mp4_enc_init(FMP4_ENC_PARAM_MY * encp, int reinit, struct enc_private * enc_pr)
{
	FMP4_ENC_PARAM_WRP* ppubw = &encp->pub;
	FMP4_ENC_PARAM * ppub = &ppubw->enc_p;

	if((ppub->u32API_version & 0xFFFFFFF0)!= (MP4E_VER&0xFFFFFFF0)) {
		printk("Fail API Version v%d.%d.%d (Current Driver v%d.%d%d)\n",
					ppub->u32API_version>>16,
					(ppub->u32API_version&0xfff0) >> 4,
					ppub->u32API_version&0x000f,
					MP4E_VER_MAJOR, MP4E_VER_MINOR, MP4E_VER_MINOR2);
		printk("Please upgrade your MPEG4 driver and re-compiler AP.\n");
		return -1;
	}
	if ((ppub->u32MaxWidth == 0) || (ppub->u32MaxHeight  == 0)) {
		ppub->u32MaxWidth = mp4_max_width;
		ppub->u32MaxHeight = mp4_max_height;
	}
	else if ((ppub->u32MaxWidth > mp4_max_width) || (ppub->u32MaxHeight > mp4_max_height)) {
		printk("input parameter over capacity (%dx%d > %dx%d)\n",
				ppub->u32MaxWidth, ppub->u32MaxHeight, mp4_max_width, mp4_max_height);
		return -1;
	}
	enc_pr->Y_sz = ((ppub->u32FrameWidth + 15)/16) * 16 * ((ppub->u32FrameHeight + 15)/16)*16;
	encp->u32VirtualBaseAddr = (unsigned int)mcp100_dev->va_base;
	encp->va_base_wrp = mcp100_dev->va_base_wrp;

	encp->u32CacheAlign= CACHE_LINE;
	encp->pu8ReConFrameCur=NULL;
	encp->pu8ReConFrameRef=NULL;
	encp->p16ACDC=NULL;
	encp->pfnDmaMalloc=fconsistent_alloc;
	encp->pfnDmaFree=fconsistent_free;
	encp->pfnMalloc = fkmalloc;
	encp->pfnFree = fkfree;
	if (reinit) {
		if (FMpeg4EncReCreate(&enc_pr->enc_handle, encp) < 0) {
			printk("FMpeg4EncReCreate return fail\n");
			return -1;
		}
	}
	else	if (enc_pr->enc_handle) {
		printk("fmp4e: previous one %d was not released", enc_pr->dev);
		return -1;
	}
	else {
		enc_pr->enc_handle = FMpeg4EncCreate(encp);
		if(&enc_pr->enc_handle == NULL) {
			printk("FMpeg4EncCreate return fail\n");
			return -1;
		}
	}
	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
long fmpeg4_encoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int fmpeg4_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	FMP4_ENC_FRAME enc_frame;
	FMP4_ENC_PARAM_MY enc_param;
	void * user_va;
	int ret=0;
	struct enc_private * encp = (struct enc_private *)filp->private_data;
	int dev = encp->dev;

	F_DEBUG("fmpeg4_encoder_ioctl:cmd=0x%x\n",cmd);
	if(((1<<dev)&enc_idx)==0)    {
		printk("No Support index %d for 0x%x\n",dev,enc_idx);
		return -EFAULT;
	}

	down(&fmutex);
	switch(cmd) {
		case FMPEG4_IOCTL_ENCODE_REINIT:
			if (copy_from_user((void *)&enc_param.pub.enc_p, (void *)arg, sizeof(FMP4_ENC_PARAM))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			enc_param.pub.img_fmt = -1;
			if (mp4_enc_init(&enc_param, 1, encp) < 0)
				ret=-EFAULT;
			break;
		case FMPEG4_IOCTL_ENCODE_INIT:
			if (copy_from_user((void *)&enc_param.pub.enc_p, (void *)arg, sizeof(FMP4_ENC_PARAM))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			enc_param.pub.img_fmt = 1;	// MP4_2D
			if (mp4_enc_init(&enc_param, 0, encp) < 0)
				ret=-EFAULT;
			break;
		case FMPEG4_IOCTL_ENCODE_INIT_264:
			if (copy_from_user((void *)&enc_param.pub.enc_p, (void *)arg, sizeof(FMP4_ENC_PARAM))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			enc_param.pub.img_fmt = 0;	// 264_2D
			if (mp4_enc_init(&enc_param, 0, encp) < 0)
				ret=-EFAULT;
			break;
#ifdef DMAWRP
		case FMPEG4_IOCTL_ENCODE_INIT_WRP:
			if (copy_from_user((void *)&enc_param.pub, (void *)arg, sizeof(FMP4_ENC_PARAM_WRP))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			if (mp4_enc_init(&enc_param, 0, encp) < 0)	// MP4_2D
				ret=-EFAULT;
			break;
#endif
		case FMPEG4_IOCTL_SET_INTER_QUANT:
			if (arg) {
				if (encp->customer_inter_quant == NULL) {
					encp->customer_inter_quant = fkmalloc(sizeof(unsigned char)*QUANT_SIZE, CACHE_LINE, CACHE_LINE);
					if ( !encp->customer_inter_quant ) {
						err("fail to fkmalloc for customer_inter_quant");
						ret=-EFAULT;
						goto encoder_ioctl_exit;
					}
				}
				if (copy_from_user((void *)encp->customer_inter_quant, (void *)arg, sizeof(unsigned char)*QUANT_SIZE)) {
					err("Error copy_from_user()");
					ret=-EFAULT;
					goto encoder_ioctl_exit;
				}
			}
			else {
				if (encp->customer_inter_quant) {
					fkfree(encp->customer_inter_quant);
					encp->customer_inter_quant = NULL;
				}
			}
			break;
		case FMPEG4_IOCTL_SET_INTRA_QUANT:	
			if (arg) {
				if (encp->customer_intra_quant) {
					encp->customer_intra_quant = fkmalloc(sizeof(unsigned char)*QUANT_SIZE, CACHE_LINE, CACHE_LINE);
					if ( !encp->customer_intra_quant ) {
						err("fail to fkmalloc for customer_inter_quant");
						ret=-EFAULT;
						goto encoder_ioctl_exit;
					}
				}
				if (copy_from_user((void *)encp->customer_intra_quant, (void *)arg, sizeof(unsigned char)*QUANT_SIZE)) {
					err("Error copy_from_user()");
					ret=-EFAULT;
					goto encoder_ioctl_exit;
				}
			}
			else {
				if (encp->customer_intra_quant) {
					fkfree(encp->customer_intra_quant);
					encp->customer_intra_quant = NULL;
				}
			}
			break;	
		case FMPEG4_IOCTL_ENCODE_FRAME:
			if(encp->enc_handle == NULL) {
				printk("No this handle %d\n", dev);
 				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			if (copy_from_user((void *)&enc_frame, (void *)arg, sizeof(enc_frame))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			user_va=enc_frame.bitstream;
			//enc_frame.bitstream=(void *)mcp100_bs_phy;  //change to PA
			//enc_frame.length = 0;
			enc_frame.bitstream=(void *)mcp100_bs_buffer.addr_pa;  //change to PA
			enc_frame.length = 0;
  			//check continued Y
  			/*
 			if(enc_frame.pu8YFrameBaseAddr!=0) {
				if(!check_continued((unsigned int)enc_frame.pu8YFrameBaseAddr, encp->Y_sz))   {
					printk("Warring! The encode Y input buffer not continued! Please use mmap instead.\n");
					ret=-EFAULT;
					goto encoder_ioctl_exit;
				}
			}
			enc_frame.pu8YFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8YFrameBaseAddr);
			enc_frame.pu8UFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8UFrameBaseAddr);
			enc_frame.pu8VFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8VFrameBaseAddr);
			*/
			enc_frame.pu8YFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa;
			enc_frame.pu8UFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa + mp4_max_width * mp4_max_height;
			enc_frame.pu8VFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa + mp4_max_width * mp4_max_height * 5 / 4;
			enc_frame.quant_inter_matrix = encp->customer_inter_quant;
			enc_frame.quant_intra_matrix = encp->customer_intra_quant;
			F_DEBUG("enc_frame.bitstream=0x%x y=0x%x u=0x%x v=0x%x\n",
				enc_frame.bitstream,enc_frame.pu8YFrameBaseAddr,
				enc_frame.pu8UFrameBaseAddr,enc_frame.pu8VFrameBaseAddr);

			if (FMpeg4EncOneFrame(encp->enc_handle, &enc_frame, 1) < 0) {
				printk("Error to encode frame!\n");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}

			if(enc_frame.length <= 0) {
				printk("Error to encode frame!len 0x%x \n", enc_frame.length);
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}

			enc_frame.frameindex = FMpeg4EncFrameCnt(encp->enc_handle);
			//if(enc_frame.length > mcp100_bs_length)    {
			if(enc_frame.length > mcp100_bs_buffer.size)    {
				printk("Error to encode frame,size overflow 0x%x!\n",enc_frame.length);
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
            #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
                __cpuc_flush_dcache_area((void *)mcp100_bs_buffer.addr_va, enc_frame.length);
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
				dma_consistent_sync((unsigned long)mcp100_bs_buffer.addr_va, (void __iomem *)mcp100_bs_buffer.addr_pa, enc_frame.length, DMA_FROM_DEVICE);
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
				consistent_sync((void *)mcp100_bs_buffer.addr_va, enc_frame.length, DMA_FROM_DEVICE);
			#endif
			if (copy_to_user((void *)user_va,(void *)mcp100_bs_buffer.addr_va, enc_frame.length)) {
				err("Error copy_to_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			if (copy_to_user((void *)arg,(void *)&enc_frame,sizeof(enc_frame))) {
				err("Error copy_to_user()");
				ret=-EFAULT;
			}
			break;
		case FMPEG4_IOCTL_ENCODE_FRAME_NO_VOL:
			if(encp->enc_handle == NULL) {
				printk("No this handle %d\n", dev);
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}

			if (copy_from_user((void *)&enc_frame, (void *)arg, sizeof(enc_frame))) {
				err("Error copy_from_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			user_va=enc_frame.bitstream;
			//enc_frame.bitstream=(void *)mcp100_bs_phy;  //change to PA
			enc_frame.bitstream=(void *)mcp100_bs_buffer.addr_pa;  //change to PA
			enc_frame.length = 0;
			//check continued Y
			/*
			if(enc_frame.pu8YFrameBaseAddr != 0) {
				if(!check_continued((unsigned int)enc_frame.pu8YFrameBaseAddr, encp->Y_sz))   {
					printk("Warring! The encode Y input buffer not continued! Please use mmap instead.\n");
					ret=-EFAULT;
					goto encoder_ioctl_exit;
				}
			}
			enc_frame.pu8YFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8YFrameBaseAddr);
			enc_frame.pu8UFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8UFrameBaseAddr);
			enc_frame.pu8VFrameBaseAddr=(unsigned char *)user_va_to_pa((unsigned int)enc_frame.pu8VFrameBaseAddr);
			*/
			enc_frame.pu8YFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa;
			enc_frame.pu8UFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa + mp4_max_width * mp4_max_height;
			enc_frame.pu8VFrameBaseAddr = (unsigned char *)mp4_enc_raw_buffer.addr_pa + mp4_max_width * mp4_max_height * 5 / 4;
			enc_frame.quant_inter_matrix = encp->customer_inter_quant;
			enc_frame.quant_intra_matrix = encp->customer_intra_quant;
			F_DEBUG("enc_frame.bitstream=0x%x y=0x%x u=0x%x v=0x%x\n",
				enc_frame.bitstream,enc_frame.pu8YFrameBaseAddr,
				enc_frame.pu8UFrameBaseAddr,enc_frame.pu8VFrameBaseAddr);
			if (FMpeg4EncOneFrame(encp->enc_handle, &enc_frame, 0) < 0) {
				printk("Error to encode frame!\n");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}

			if(enc_frame.length <= 0) {
				printk("Error to encode frame!len 0x%x \n", enc_frame.length);
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			enc_frame.frameindex=FMpeg4EncFrameCnt(encp->enc_handle);
			if(enc_frame.length > mcp100_bs_buffer.size)    {
				printk("Error to encode frame,size overflow 0x%x!\n",enc_frame.length);
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
            #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33))
                __cpuc_flush_dcache_area((void *)mcp100_bs_buffer.addr_va, enc_frame.length);
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24))
				dma_consistent_sync((unsigned long)mcp100_bs_buffer.addr_va, (void __iomem *)mcp100_bs_buffer.addr_pa, enc_frame.length, DMA_FROM_DEVICE);
			#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
				consistent_sync((void *)mcp100_bs_buffer.addr_va, enc_frame.length, DMA_FROM_DEVICE);
			#endif
			if (copy_to_user((void *)user_va,(void *)mcp100_bs_buffer.addr_va,enc_frame.length)) {
				err("Error copy_to_user()");
				ret=-EFAULT;
				goto encoder_ioctl_exit;
			}
			if (copy_to_user((void *)arg,(void *)&enc_frame,sizeof(enc_frame))) {
				err("Error copy_to_user()");
				ret=-EFAULT;
			}
 			break;	
		case FMPEG4_IOCTL_ENCODE_INFO:
			if(encp->enc_handle == NULL) {
				printk("No this handle %d\n", dev);
				ret=-EFAULT;
			}
			else {
				MACROBLOCK_INFO *mbinfo = FMpeg4EncGetMBInfo(encp->enc_handle);
				if (copy_to_user((void *)arg,(void *)mbinfo,sizeof(MACROBLOCK_INFO)*encp->Y_sz/256)) {
					err("Error copy_to_user()");
					ret=-EFAULT;
				}
			}
			break;
		default:
			printk("Not support this IOCTL %x \n", cmd);
			ret=-EFAULT;
			break;
	}

encoder_ioctl_exit:
	up(&fmutex);
	return ret;
}


int fmpeg4_encoder_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if 0
	unsigned int size = vma->vm_end - vma->vm_start;  
	unsigned int yuv_size = DIV_M10000(mp4_max_width * mp4_max_height * 2);		

	//printk(KERN_DEBUG "fmpeg4_encoder_mmap Alloc va 0x%x size 0x%x\n",(unsigned int)vma->vm_start,(unsigned int)size);
	if(size > yuv_size) {
		printk("The mmap size %d must less than %d bytes\n", size, yuv_size);
		return -EFAULT;
	}

	if (0 == enc_raw_yuv_virt) {
		enc_raw_yuv_virt = (unsigned int)fcnsist_alloc(PAGE_ALIGN(yuv_size),
			(void **)&enc_raw_yuv_phy, 0/*ncnb*/);
		if (0 == enc_raw_yuv_virt) {
			printk("Fail to allocate encode raw buffer!\n");
			return -EFAULT;
		}
	}

	vma->vm_pgoff = enc_raw_yuv_phy >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
    
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
		fcnsist_free (PAGE_ALIGN(yuv_size), (void *)enc_raw_yuv_virt, (void *)enc_raw_yuv_phy);
		return -EAGAIN;
	}
#else
    unsigned int size = vma->vm_end - vma->vm_start; 
    vma->vm_pgoff = mp4_enc_raw_buffer.addr_pa >> PAGE_SHIFT;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= VM_RESERVED;
    
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
        //fcnsist_free (PAGE_ALIGN(yuv_size), (void *)enc_raw_yuv_virt, (void *)enc_raw_yuv_phy);
        return -EAGAIN;
    }
#endif
	return 0;
}


int fmpeg4_encoder_release(struct inode *inode, struct file *filp)
{
	struct enc_private * encp = (struct enc_private *)filp->private_data;
	int dev = encp->dev;

	down(&fmutex);
	if(encp->enc_handle)
		FMpeg4EncDestroy(encp->enc_handle);
	filp->private_data=0;

	if(encp->customer_inter_quant)
		fkfree(encp->customer_inter_quant);
	if(encp->customer_intra_quant)
		fkfree(encp->customer_intra_quant);
	enc_idx &= ~(ENC_IDX_MASK_n(dev));	
	up(&fmutex);

	return 0;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
static struct file_operations fmpeg4_encoder_fops = {
	.owner = THIS_MODULE,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
    .unlocked_ioctl = fmpeg4_encoder_ioctl,
#else
	.ioctl = fmpeg4_encoder_ioctl,
#endif
	.mmap = fmpeg4_encoder_mmap,
	.open = fmpeg4_encoder_open,
	.release = fmpeg4_encoder_release,
};

static struct miscdevice fmpeg4_encoder_dev = {
	.minor = FMPEG4_ENCODER_MINOR,
	.name = "fenc",
	.fops = &fmpeg4_encoder_fops,
};
#else
static struct file_operations fmpeg4_encoder_fops = {
	.owner:      THIS_MODULE,
	.ioctl:		fmpeg4_encoder_ioctl,
	.mmap:       fmpeg4_encoder_mmap,
	.open:		fmpeg4_encoder_open,
	.release:	fmpeg4_encoder_release,
};
static struct miscdevice fmpeg4_encoder_dev = {
	.minor: FMPEG4_ENCODER_MINOR,
	.name: "fmpeg4 encoder",
	.fops: &fmpeg4_encoder_fops,
};
#endif

static void mp4e_isr(int irq, void *dev_id, unsigned int base)
{
	MP4_COMMAND type;
	volatile Share_Node *node = (volatile Share_Node *) (0x8000 + base);		 

	type = (node->command & 0xF0)>>4;
	if( ( type == ENCODE_IFRAME) || (type == ENCODE_PFRAME) || (type == ENCODE_NFRAME) ) {
		#ifdef EVALUATION_PERFORMANCE
			encframe.hw_stop = get_counter();
		#endif
	#ifndef GM8120
			*(volatile unsigned int *)(base + 0x20098)=0x80000000;
		#endif
	}
}

extern void switch2mp4e(void * codec, ACTIVE_TYPE curr);
static mcp100_rdev mp4edv = {
	.handler = mp4e_isr,
	.dev_id = NULL,
	.switch22 = switch2mp4e,
};

//int drv_mp4e_init(void)
int mp4e_drv_init(void)
{
	if (mcp100_register(&mp4edv, TYPE_MPEG4_ENCODER, MP4E_MAX_CHANNEL) < 0)
		return -1;
	if(misc_register(&fmpeg4_encoder_dev) < 0){
		err("fmpeg4_encoder_dev misc-register fail");
		mcp100_deregister (TYPE_MPEG4_ENCODER);
		return -1;
	}
    if (allocate_buffer(&mp4_enc_raw_buffer, mp4_max_width * mp4_max_height * 2) < 0) {
        err("allocate fmpeg4 raw data error\n");
        return -1;
    }
	return 0;
} 

//void drv_mp4e_exit(void)
void mp4e_drv_close(void)
{
/*
	unsigned int yuv_size= DIV_M10000(mp4_max_width * mp4_max_height * 2);

	if (enc_raw_yuv_virt)
		fcnsist_free (PAGE_ALIGN(yuv_size), (void *)enc_raw_yuv_virt, (void *)enc_raw_yuv_phy);
*/
    free_buffer(&mp4_enc_raw_buffer);
	mcp100_deregister (TYPE_MPEG4_ENCODER);
	misc_deregister(&fmpeg4_encoder_dev);
}
