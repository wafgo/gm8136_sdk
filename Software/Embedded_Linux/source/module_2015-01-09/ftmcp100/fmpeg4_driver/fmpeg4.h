/* fmpeg4.h */
#ifndef _FMPEG4_H_
#define _FMPEG4_H_

//#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/ioport.h>       /* request_region */
#include <linux/interrupt.h>    /* mark_bh */
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/page.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	//#include <mach/platform/spec.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	#include <asm/arch/platform/spec.h>
#else
	#include <asm/arch/cpe/cpe.h>
#endif
/*
#define FMPEG4_IOCTL_DECODE_INIT		0x4170
#define FMPEG4_IOCTL_DECODE_FRAME 	0x4172
#define FMPEG4_IOCTL_DECODE_422		0x4174

#define FMPEG4_IOCTL_ENCODE_INIT		0x4173
#define FMPEG4_IOCTL_ENCODE_FRAME		0x4175
#define FMPEG4_IOCTL_ENCODE_INFO		0x4176
#define FMPEG4_IOCTL_ENCODE_FRAME_NO_VOL 0x4178
#define FMPEG4_IOCTL_SET_INTER_QUANT	0x4184
#define FMPEG4_IOCTL_SET_INTRA_QUANT	0x4185
*/
#define             FMPEG4_DECODER_MINOR    20 //(10,20)
#define             FMPEG4_ENCODER_MINOR    21 //(10,21)

#define DIV_M10000(n) ((n+0xFFFF)/0x10000)*0x10000
#define DIV_M1000(n) ((n+0xFFF)/0x1000)*0x1000

#ifdef RESOLUTION_2592x1944
#define MAX_DEFAULT_WIDTH 2592
#define MAX_DEFAULT_HEIGHT 1944
//#define 		BIT_STREAM_SIZE             0x740000 	//bit stream size should < 2C0K
//#define             YUV_SIZE                    	     0x740000     		//1600x1200 need 0x2BF200
#elif defined( RESOLUTION_1600x1200)
#define MAX_DEFAULT_WIDTH 1600
#define MAX_DEFAULT_HEIGHT 1200
//#define 		BIT_STREAM_SIZE             0x2C0000 	//bit stream size should < 2C0K
//#define             YUV_SIZE                    	     0x2C0000     		//1600x1200 need 0x2BF200
#elif defined(RESOLUTION_720x576)
#define MAX_DEFAULT_WIDTH 720
#define MAX_DEFAULT_HEIGHT 576
//#define             BIT_STREAM_SIZE             0xa0000   	//bit stream size should < 200K
//#define             YUV_SIZE                    	    0xa0000     		//720x576 need 0x97e00
#endif


extern int fmpeg4_encoder_open(struct inode *inode, struct file *filp);
//extern int fmpeg4_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
extern long fmpeg4_encoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
extern int fmpeg4_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
extern int fmpeg4_encoder_mmap(struct file *file, struct vm_area_struct *vma);
extern int fmpeg4_encoder_release(struct inode *inode, struct file *filp);
extern int fmpeg4_decoder_open(struct inode *inode, struct file *filp);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
extern long fmpeg4_decoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
extern int fmpeg4_decoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
extern int fmpeg4_decoder_mmap(struct file *file, struct vm_area_struct *vma);
extern int fmpeg4_decoder_release(struct inode *inode, struct file *filp);

#if 0
#define F_DEBUG(fmt, args...) printk("FMEG4: " fmt, ## args)
#else
#define F_DEBUG(a...)
#endif

#endif
