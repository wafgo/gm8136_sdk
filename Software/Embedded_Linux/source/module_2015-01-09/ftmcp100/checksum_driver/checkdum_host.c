#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>
#endif
#ifdef TWO_P_EXT
#include "../ioctl_cs.h"
//#include "checksum_ex.h"
#endif

#include "checksum_node.h"

#include "../fmcp_type.h"
#include "../fmpeg4_driver/common/dma.h"
//#include "checksum_ex.h"
#if defined(TWO_P_INT)|defined(HOST_MODE)
#include "checksum_table.h"
#endif
//#ifdef TWO_P_INT
//#include "checksum_int.h"
//#endif
//#include "osd.h"
//#include "osd_ext.h"
//#include "fmpeg4_driver/common/share_mem.h"
//#include "fmpeg4_driver/common/dma.h"

// osd start
#define REG_OFFSET      0x20000
#define BANK_OFFSET     0x10000
#define MP4_OFF         (BANK_OFFSET + 0x10000)
#define DMA_OFF         (MP4_OFF + 0x400)
#define SHAREMEM_OFF    (0x8000)

#define mDmaLocMemAddr14b(v)    ((((unsigned int)v)>>1)&0x0fffffffc)

#define MAX_STEP    0x800

#define DMACMD_IN_0     (BANK_OFFSET + 0x000)   // 0x000 ~ 0x010
#define DMACMD_IN_1     (DMACMD_IN_0 + 0x80)   // 0x080 ~ 0x090

#define BG_0    (BANK_OFFSET + 0x8000)  // 0x4000 ~ 0x5000
#define BG_1    (BG_0 + 0x800)         // 0x5000 ~ 0x6000
#define OBJ_0   (BANK_OFFSET + 0x4000)
#define OBJ_1   (OBJ_0 + 0x400)

#ifndef TWO_P_INT
	#define CSBase(posdhw)		(posdhw->base_address)
#else
	#define CSBase(posdhw)		0
#endif

int gLogIdx;

#ifdef TWO_P_EXT // run on external host CPU
extern void mcp100_switch(void * codec, ACTIVE_TYPE type);

#ifdef HOST_MODE
void cs_start(cs_hw * posdhw);
#endif

int cs_tri(FMCP_CS_PARAM *posdin) 
{
    Share_Node_CS *node = (Share_Node_CS *)(posdin->ip_base + SHAREMEM_OFF);
	cs_hw * posdhw = &node->cshw;

	// wait "wait for command"
	if ( (node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("checksum is not issued due to mcp100 is busy, current command %d\n", node->command & 0x0F);
		return -1;
	}
    posdhw->base_address = posdin->ip_base;
    posdhw->buf_base = posdin->buffer_pa;
    posdhw->len = posdin->buffer_len;
    posdhw->type = posdin->type;
    mcp100_switch (NULL, TYPE_CHECK_SUM);
//#ifndef TWO_P_EXT
#ifdef HOST_MODE
    cs_start(posdhw);
    printk("result = 0x%x\n", node->cshw.result);
	return 0;
#endif
	node->command = CHECKSUM;    // trigger MCP100
	return 0;
}
#endif  // TWO_P_EXT

//#ifdef TWO_P_INT
#if defined(TWO_P_INT)|defined(HOST_MODE)
#define CS_BIT_IN       (1L<<0)
#define CS_BIT_CS       (1L<<1)
#define CS_BIT_CS_DONE  (1L<<2)

#define LOAD_STEP       MAX_STEP
static void dma_init_cs(uint32_t base)
{
	uint32_t * dma_cmd[2];
	dma_cmd[0] = (uint32_t *)(base + DMACMD_IN_0);
	dma_cmd[1] = (uint32_t *)(base + DMACMD_IN_1);
 	// init dma parameter
	dma_cmd[0][0 + 1] = mDmaLocMemAddr14b (BG_0);
	dma_cmd[1][0 + 1] = mDmaLocMemAddr14b (BG_1);
	dma_cmd[0][0 + 2] =
	dma_cmd[1][0 + 2] = 0;

    dma_cmd[0][4 + 1] = mDmaLocMemAddr14b (OBJ_0);
    dma_cmd[1][4 + 1] = mDmaLocMemAddr14b (OBJ_1);
    dma_cmd[0][4 + 2] = 
    dma_cmd[1][4 + 2] = 0;
}

#ifdef HOST_MODE
void dump_dma(uint32_t base)
{
    uint32_t * dma_cmd = (uint32_t *)base;
    printk("base 0x%x\n", base);
    printk("[0] 0x%x\n", dma_cmd[0]);
    printk("[1] 0x%x\n", dma_cmd[1]);
    printk("[2] 0x%x\n", dma_cmd[2]);
    printk("[3] 0x%x\n", dma_cmd[3]);
}

void dump_dma_reg(volatile MDMA *pmdma)
{
    printk("00: %x\n",pmdma->SMaddr);     // 00
    printk("04: %x\n",pmdma->LMaddr);      // 04
    printk("08: %x\n",pmdma->BlkWidth);        // 08
    printk("0C: %x\n",pmdma->Control);     // 0C
    printk("10: %x\n",pmdma->CCA);         // 10
    printk("14: %x\n",pmdma->Status);      // 14
    printk("18: %x\n",pmdma->CCA2);        // 18   new feature from 2004 0507
    printk("1C: %x\n",pmdma->GRPC);        // 1C   Group execution control
    printk("20: %x\n",pmdma->GRPS);        // 20   Group sync control
    printk("24: %x\n",pmdma->ABFCtrl); 
}
#endif

typedef struct
{
	int pos;        // current x, unit: pel
	int toggle;		// indicate which table will be used;
	int step;		// current step, unit: pel
} CS_mb;


// check sum
unsigned int UTL_crc(unsigned char *ptr, unsigned int len, unsigned initial_pattern)
{
    unsigned int index;
    unsigned int crc = initial_pattern;
    for(index=0;index<len;index++) {
        crc = DHAV_crcTable[(crc^ptr[index])&0xff] ^ (crc>>8);
    }
    return crc;
}

unsigned int UTL_sumByte(unsigned char *ptr, unsigned int len, unsigned int initial_pattern)
{
    unsigned int index;
    unsigned int sum = initial_pattern;
    for (index = 0; index < len;index++ )
    {
        sum += *ptr++;
    }    
    return sum;
}

unsigned int UTL_sumFourByte(unsigned char *ptr, unsigned int len, unsigned int initial_pattern)
{
    unsigned int index;
    unsigned int sum = initial_pattern;
    unsigned int fourByteArray = (len>>2);
    unsigned int leftByte = len - (fourByteArray <<2);
    unsigned int *pInt = NULL;
    unsigned int mask[4] = {0x0, 0x000000ff, 0x0000ffff, 0x00ffffff};

    pInt = (unsigned int *)ptr;
    for (index = 0; index < fourByteArray;index++ ) {
        sum += *pInt++ ;
    }
    if (leftByte) {
        sum += *pInt & mask[leftByte];
    }
    return sum;
}

//#define DEBUG_CS
#define SEQUENCE_PROCESS
// 1. dma in
// 2. compute check
void cs_start(cs_hw * posdhw)  
{
    volatile MDMA *pmdma = (volatile MDMA *)(CSBase(posdhw) + DMA_OFF);
    uint32_t * dma_cmd;// = (uint32_t *)(CSBase(posdhw) + DMACMD_IN_0);
    int pos = 0, dma_step, cs_step = 0, dma_id = 0, cs_id = -1;
    unsigned char *ptr;
    unsigned int initial_pattern;
    //int idx = 0, i;

    if (1 == posdhw->type)      // crc
        initial_pattern = 0xFFFFFFFF;
    else
        initial_pattern = 0;

    dma_init_cs(CSBase(posdhw));
#ifdef SEQUENCE_PROCESS
    cs_id = 0;
    dma_step = (posdhw->len + LOAD_STEP - 1)/LOAD_STEP*LOAD_STEP;
    //pos = 0;
    //while (1) {
    for (pos = 0; pos < dma_step; pos += LOAD_STEP) {
        cs_step = posdhw->len - pos;
        if (cs_step > LOAD_STEP)
            cs_step = LOAD_STEP;
// 1) trigger dma
        dma_cmd = (uint32_t *)(CSBase(posdhw) + DMACMD_IN_0);
        dma_cmd[0] = posdhw->buf_base + pos;
#if 0
        dma_cmd[3] =(0 << 26) | //mask interrupt
                    (1 << 23) |     //dma_en
                    (0 << 21) |     //chain enable
                    (0 << 20) |     //sys->local
                    (0 << 18) |     //local: seq
                    (0 << 16) |     //sys:seq
                    (LOAD_STEP/4);  // always be max size
        pmdma->CCA = DMACMD_IN_0 | 0x02;
        pmdma->Control =    (0x00A00000); //start DMA from local memory
#else
        dma_cmd[3] =(1 << 26) |     //mask interrupt
                    (1 << 23) |     //dma_en
                    (1 << 21) |     //chain enable
                    (0 << 20) |     //sys->local
                    (0 << 18) |     //local: seq
                    (0 << 16) |     //sys:seq
                    (LOAD_STEP/4);  // always be max size
        dma_cmd[4] = posdhw->buf_base + pos;
        dma_cmd[7] =(0 << 26) | //mask interrupt
                    (1 << 23) |     //dma_en
                    (0 << 21) |     //chain enable
                    (0 << 20) |     //sys->local
                    (0 << 18) |     //local: seq
                    (0 << 16) |     //sys:seq
                    (0);  // always be max size
        pmdma->CCA = DMACMD_IN_0 | 0x02;
        pmdma->Control =    (0x04A00000); //start DMA from local memory
#endif
// 2) wait dma done
        while(!(pmdma->Status & 0x1))
                  ;
// 3) compute check sum
        ptr = (unsigned char *)(BG_0+ CSBase(posdhw));
        // calculate check sum
        if (1 == posdhw->type) {    // crc
            initial_pattern = UTL_crc(ptr, cs_step, initial_pattern);
        }
        else if (2 == posdhw->type) {   // sum
            initial_pattern = UTL_sumByte(ptr, cs_step, initial_pattern);
        }
        else if (3 == posdhw->type) {   // sum four byte
            initial_pattern = UTL_sumFourByte(ptr, cs_step, initial_pattern);
        }
    }
#else
    while (1) {
        dma_step = posdhw->len - pos;
        if (dma_step > LOAD_STEP)
            dma_step = LOAD_STEP;
// 1) trigger dma
        if (dma_id >= 0) {
            if (dma_id)
                dma_cmd = (uint32_t *)(CSBase(posdhw) + DMACMD_IN_1);
            else
                dma_cmd = (uint32_t *)(CSBase(posdhw) + DMACMD_IN_0);
            dma_cmd[0] = posdhw->buf_base + pos;
            dma_cmd[3] =(0 << 26) | //mask interrupt
            			(1 << 23) | 	//dma_en
            			(0 << 21) | 	//chain enable
            			(0 << 20) | 	//sys->local
            			(0 << 18) | 	//local: seq
            			(0 << 16) | 	//sys:seq
            			(LOAD_STEP/4);  // always be max size
            			/*((dma_step+3)/4);*/
            if (dma_id)
                pmdma->CCA = DMACMD_IN_1 | 0x02;
            else
                pmdma->CCA = DMACMD_IN_0 | 0x02;
            pmdma->Control =    (0x00A00000); //start DMA from local memory
        }
        // compute
// 2) compute check sum
        if (cs_id >= 0) {
            if (cs_id)
                ptr = (unsigned char *)(BG_1+ CSBase(posdhw));
            else
                ptr = (unsigned char *)(BG_0+ CSBase(posdhw));
            // calculate check sum
            if (1 == posdhw->type) {    // crc
                initial_pattern = UTL_crc(ptr, cs_step, initial_pattern);
            }
            else if (2 == posdhw->type) {   // sum
                initial_pattern = UTL_sumByte(ptr, cs_step, initial_pattern);
            }
            else if (3 == posdhw->type) {   // sum four byte
                initial_pattern = UTL_sumFourByte(ptr, cs_step, initial_pattern);
                //if (idx < 0x100)
                //    posdhw->data[idx++] = initial_pattern;
            }
        }
// 3) wait dma done
        if (dma_id >= 0) {
            while(!(pmdma->Status & 0x1))
                  ;
        }
        cs_step = dma_step;
        cs_id = dma_id;
        dma_id ^= 1;
        if ((pos + cs_step) >= posdhw->len)
            dma_id = -1;
        else
            pos += LOAD_STEP;
        if (cs_id < 0 && dma_id < 0)
            break;
    }
#endif    
    if (1 == posdhw->type)      // crc
        posdhw->result = ~initial_pattern;
    else
        posdhw->result = initial_pattern;
    return;
}


#endif
