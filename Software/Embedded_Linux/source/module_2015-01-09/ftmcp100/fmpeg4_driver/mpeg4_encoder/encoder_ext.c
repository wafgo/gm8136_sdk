#define ENCODER_EXT_GLOBAL

#ifdef LINUX
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
#include <asm/io.h>
#include <asm/uaccess.h>
#include "../common/define.h"
#include "../common/share_mem.h"
#include "encoder_ext.h"
#include "../common/local_mem.h"
#else
#include "define.h"
#include "share_mem.h"
#include "encoder_ext.h"
#include "local_mem.h"
#endif

static void encoder_pass_params(Encoder_x * enc,Share_Node_Enc *node)
{
	node->enc       = *(Encoder_x *)enc;
	node->current1  = *(enc->Enc.current1);
	node->reference = *(enc->Enc.reference);
}

static void encoder_restore_params(Encoder_x * enc,Share_Node_Enc *node)
{
	FRAMEINFO *current1  = enc->Enc.current1;
	FRAMEINFO *reference = enc->Enc.reference;
	*(Encoder_x *)enc = node->enc;
	// since some variables are changed in embedded CPU's firmware
	// we should reflect the changes back to the enc object
	current1->coding_type = node->current1.coding_type;
	current1->bRounding_type = node->current1.bRounding_type;

	// since the pointer of current1 and reference are changed during
	// calling encoder_pass_params(), so we must restore it
	enc->Enc.current1  = current1;
	enc->Enc.reference = reference;
}

static int encoder_ipn_tri(Encoder_x * enc, MP4_COMMAND command)
{
	Share_Node_Enc *node = (Share_Node_Enc *)(enc->Enc.u32VirtualBaseAddr + SHAREMEM_OFF);
	// check "wait for command"	
	if  ((node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("command (%d) is not issued due to mcp100 is busy\n", command);
		return -1;
	}
	node->int_enabled=ENABLE_INTERRUPT;
	encoder_pass_params(enc,node);
	node->command = command;    // trigger MCP100
	return 0;
}


int encoder_iframe_tri(Encoder_x * enc)
{
	return encoder_ipn_tri (enc,ENCODE_IFRAME);
}

int encoder_pframe_tri(Encoder_x * enc)
{
	return encoder_ipn_tri (enc,ENCODE_PFRAME);
}

int encoder_nframe_tri(Encoder_x * enc)
{
	return encoder_ipn_tri (enc, ENCODE_NFRAME);
}

int encoder_frame_sync(Encoder_x * enc)
{
	Share_Node_Enc *node = (Share_Node_Enc *)(enc->Enc.u32VirtualBaseAddr + SHAREMEM_OFF);
	// check "wait for command"
	if  ((node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("mcp100 still busy when encoder_ipn_sync\n");
		return -1;
	}
	node->int_flag=NO_INTERRUPT;
	encoder_restore_params(enc,node);
	return 0;
}

#ifndef SUPPORT_VG
extern 	wait_queue_head_t mcp100_codec_queue;
int encoder_frame_block(Encoder_x * enc)
{
	Share_Node_Enc *node = (Share_Node_Enc *)(enc->Enc.u32VirtualBaseAddr + SHAREMEM_OFF);
	int hw_enc_time = (enc->Enc.mb_width * enc->Enc.mb_height + 1349) / 1350;	// 1350 = 720*480/256
	hw_enc_time = (hw_enc_time > 5) ? 5: hw_enc_time;
	#ifdef FIE8150	// FPGA
	hw_enc_time = 10;
	#endif
	wait_event_timeout(mcp100_codec_queue, (node->command&0x0F) == WAIT_FOR_COMMAND, msecs_to_jiffies(hw_enc_time * 100));
	if ((node->command&0x0F) != WAIT_FOR_COMMAND) {
		printk("mp4 encoder time out, 0x%x\n", node->command);
		return -1;
	}
	return 0;
}
#endif

void encoder_em_init(uint32_t base)
{
	Share_Node_Enc *node = (Share_Node_Enc *)(base + SHAREMEM_OFF);
	node->command = WAIT_FOR_COMMAND;
}

