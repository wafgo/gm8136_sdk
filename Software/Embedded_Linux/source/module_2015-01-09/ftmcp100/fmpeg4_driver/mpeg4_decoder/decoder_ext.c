#define DECODER_EXT_GLOBAL
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
#include "decoder_ext.h"
#include "../common/local_mem.h"
#else
#include "define.h"
#include "share_mem.h"
#include "decoder_ext.h"
#include "local_mem.h"
#endif

#ifdef EVALUATION_PERFORMANCE
#include "../common/performance.h"
static struct timeval tv_init, tv_curr;
extern FRAME_TIME decframe;
extern TOTAL_TIME dectotal;
extern uint64 time_delta(struct timeval *start, struct timeval *stop);
extern void dec_performance_count(void);
extern void dec_performance_report(void);
extern void dec_performance_reset(void);
extern unsigned int get_counter(void);
#endif

static int decoder_ipn_tri(DECODER_ext * dec,
			   Bitstream * bs,
			   int rounding,
			   MP4_COMMAND command)
{
	Share_Node_Dec *node = (Share_Node_Dec *)(dec->u32VirtualBaseAddr + SHAREMEM_OFF);

	// wait "wait for command"
	if ( (node->command&0x0F) != WAIT_FOR_COMMAND) {
		printk ("decoder_ipn_tri: command (%d) is not issued due to mcp100 is busy\n", command);
		return -1;
	}

	#ifdef POLLING
		node->int_enabled=DISABLE_INTERRUPT;
		node->int_flag=NO_INTERRUPT;
	#else
		node->int_enabled=ENABLE_INTERRUPT;
	#endif

	node->dec = *(DECODER_int *)dec;
	if(command!=DECODE_NFRAME)
		node->bs = *bs;
	node->rounding = rounding;
	node->command = command;    // trigger MCP100
	return 0;
}

int decoder_iframe_tri(DECODER_ext * dec, Bitstream * bs)
{
	return decoder_ipn_tri (dec, bs, DONT_CARE, DECODE_IFRAME);
}

int decoder_pframe_tri(DECODER_ext * dec, Bitstream * bs, int rounding)
{
	return decoder_ipn_tri (dec, bs, rounding, DECODE_PFRAME);
}

int decoder_nframe_tri(DECODER_ext * dec)
{
	return decoder_ipn_tri (dec, DONT_CARE, DONT_CARE, DECODE_NFRAME);
}
int decoder_frame_sync(DECODER_ext * dec, Bitstream * bs)
{
	Share_Node_Dec *node = (Share_Node_Dec *)(dec->u32VirtualBaseAddr + SHAREMEM_OFF);
	if ((node->command&0x0F) != WAIT_FOR_COMMAND) {
		printk("mp4 decoder time out, 0x%x\n", node->command);
		return -1;
	}
	node->int_flag=NO_INTERRUPT;
	//if(command!=DECODE_NFRAME)
	*bs = node->bs;
	*(DECODER_int *)dec = node->dec;
	return 0;
}

#ifndef SUPPORT_VG
extern 	wait_queue_head_t mcp100_codec_queue;
int decoder_frame_block(DECODER_ext * dec)
{
	Share_Node_Dec *node = (Share_Node_Dec *)(dec->u32VirtualBaseAddr + SHAREMEM_OFF);
	int hw_dec_time = (dec->mb_width * dec->mb_height + 1349) / 1350;	// 1350 = 720*480/256
	hw_dec_time = (hw_dec_time > 5) ? 5: hw_dec_time;
	#ifdef FIE8150	// FPGA
	hw_dec_time = 10;
	#endif
	wait_event_timeout(mcp100_codec_queue, (node->command&0x0F) == WAIT_FOR_COMMAND, msecs_to_jiffies(hw_dec_time * 100));
	if ((node->command&0x0F) != WAIT_FOR_COMMAND) {
		printk("mp4 decoder time out, 0x%x\n", node->command);
		return -1;
	}
	return 0;
}
#endif
void decoder_em_init(uint32_t base)
{
	Share_Node_Dec *node = (Share_Node_Dec *)(base + SHAREMEM_OFF);
	node->command = WAIT_FOR_COMMAND;
}

