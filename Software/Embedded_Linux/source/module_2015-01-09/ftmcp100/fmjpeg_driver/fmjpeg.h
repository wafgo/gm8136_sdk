/* fmjpeg.h */
#ifndef _FMJPEG_H_
#define _FMJPEG_H_

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
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
	//#include <mach/platform/spec.h>
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	#include <asm/arch/platform/spec.h>
#else
	#include <asm/arch/cpe/cpe.h>
#endif

#define     FMJPEG_DECODER_MINOR    		40 //(10,20)
#define     FMJPEG_ENCODER_MINOR    		41 //(10,21)

#define DIV_J10000(n) ((n+0xFFFF)/0x10000)*0x10000
#define DIV_J1000(n) ((n+0xFFF)/0x1000)*0x1000
#ifdef RESOLUTION_2592x1944
#define MAX_DEFAULT_WIDTH  2592
#define MAX_DEFAULT_HEIGHT 1944
#elif defined( RESOLUTION_1600x1200)
#define MAX_DEFAULT_WIDTH  1600
#define MAX_DEFAULT_HEIGHT 1200
#elif defined( RESOLUTION_720x576)
#define MAX_DEFAULT_WIDTH  720
#define MAX_DEFAULT_HEIGHT 576
#endif
extern int fmjpeg_encoder_open(struct inode *inode, struct file *filp);
//extern int fmjpeg_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
extern long fmjpeg_encoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
extern int fmjpeg_encoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
extern int fmjpeg_encoder_mmap(struct file *file, struct vm_area_struct *vma);
extern int fmjpeg_encoder_release(struct inode *inode, struct file *filp);
extern int fmjpeg_decoder_open(struct inode *inode, struct file *filp);
//extern int fmjpeg_decoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
extern long fmjpeg_decoder_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
extern int fmjpeg_decoder_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
extern int fmjpeg_decoder_mmap(struct file *file, struct vm_area_struct *vma);
extern int fmjpeg_decoder_release(struct inode *inode, struct file *filp);


#if 1
#define F_DEBUG(fmt, args...) printk("FMJPEG: " fmt, ## args)
#else
#define F_DEBUG(a...)
#endif


#endif
