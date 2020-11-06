#ifdef LINUX
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/version.h>
#endif
#ifdef TWO_P_EXT
    #include "../ioctl_rotation.h"
#endif

#include "rotation_node.h"

#include "video_entity.h"
#include "../fmcp_type.h"
#include "../fmpeg4_driver/common/dma.h"

/*
Bank0:   0x10000
Bank1:   0x14000
Bank2/3: 0x18000
Bank4:   0x17C00
Bank5:   0x1A000
*/
#define BANK_OFFSET     0x10000
#define MP4_OFF         (BANK_OFFSET + 0x10000)
#define DMA_OFF         (MP4_OFF + 0x400)

//#define DMACMD_IN_0     (BANK_OFFSET + 0x000)   // 0x000 ~ 0x010
//#define DMACMD_IN_1     (DMACMD_IN_0 + 0x80)   // 0x080 ~ 0x090

#define DMACMD_OUT_0    (BANK_OFFSET)  // 0x4000 ~ 0x4010
#define DMACMD_IN_0     (DMACMD_OUT_0 + 0x10)   // 0x4010 ~ 0x4020
#define DMACMD_IN2_0    (DMACMD_OUT_0 + 0x20)   // 0x4010 ~ 0x4020
#define DMACMD_OUT_1    (DMACMD_OUT_0 + 0x80)   // 0x4080 ~ 0x4090
#define DMACMD_IN_1     (DMACMD_OUT_0 + 0x90)   // 0x4090 ~ 0x40A0
#define DMACMD_IN2_1    (DMACMD_OUT_0 + 0xA0)   // 0x4090 ~ 0x40A0

#define SQUARE_BLOCK
#ifdef SQUARE_BLOCK
    #define BLOCK_WIDTH     16
    #define BLOCK_WIDTH2    32
    #define BLOCK_HEIGHT    16
    #define BLOCK_HEIGHT2   32
    #define LOC_IN_0    (BANK_OFFSET + 0x100)   // 16*16*2*2
    #define LOC_OUT_0   (LOC_IN_0 + 0x400)      // 16*16*2*2
    #define LOC_IN_1    (LOC_IN_0 + 0x800)      // 16*16*2*2
    #define LOC_OUT_1   (LOC_IN_0 + 0xC00)      // 16*16*2*2
#else
    #define BLOCK_WIDTH     64
    #define BLOCK_HEIGHT    16
    #define LOC_IN_0    (BANK_OFFSET + 0x100)           // 16*16*2*4
    #define LOC_OUT_0   (LOC_IN_0 + 0x0800)     // 16*16*2*4
    #define LOC_IN_1    (LOC_IN_0 + 0x1000)     // 16*16*2*4
    #define LOC_OUT_1   (LOC_IN_1 + 0x1800)     // 16*16*2*4
#endif

#define SHAREMEM_OFF    (0x8000)

#ifndef TWO_P_INT
	#define R90DBase(posdhw)        (posdhw->base_address)
#else
    #define R90DBase(posdhw)        0
#endif

#ifdef TWO_P_EXT // run on external host CPU
extern void mcp100_switch(void * codec, ACTIVE_TYPE type);

#ifdef HOST_MODE
    void rotation_start(rt_hw * posdhw);
#endif

int FRotation_Trigger(FMCP_RT_PARAM *rt_param)
{
    Share_Node_RT *node = (Share_Node_RT *)(rt_param->u32BaseAddress + SHAREMEM_OFF);
	rt_hw * posdhw = &node->rthw;

	// wait "wait for command"
	if ( (node->command & 0x0F) != WAIT_FOR_COMMAND) {
		printk ("rotation is not issued due to mcp100 is busy, current command %d\n", node->command&0xFF);
		return -1;
	}
    posdhw->base_address = rt_param->u32BaseAddress;
    posdhw->in_buf_base = rt_param->u32InputBufferPhy;
    posdhw->out_buf_base = rt_param->u32OutputBufferPhy;
    posdhw->width = rt_param->u32Width;
    posdhw->height = rt_param->u32Height;
    posdhw->clockwise = rt_param->u32Clockwise;
    mcp100_switch (NULL, TYPE_MCP100_ROTATION);
#ifdef HOST_MODE
    rotation_start(posdhw);
	return 0;
#endif
	node->command = ROTATION;    // trigger MCP100
	return 0;
}
#endif  // TWO_P_EXT

//#ifdef TWO_P_INT
#if defined(TWO_P_INT)|defined(HOST_MODE)

#define BIT_IN      (1L<<0)
#define BIT_RT      (1L<<1)
#define BIT_OUT     (1L<<2)
#define BIT_RT_DONE (1L<<3)

#define mDmaLocMemAddr14b(v)    ((((uint32_t)(v)>>3)<<2) & 0xFFFC)  // Local memory start addr. (8uint8_ts)
#define mDmaSysWidth6b(v)       ((uint32_t)(v) & 0x3F)                  // System block width (1unit: 8 uint8_ts)
#define mDmaSysOff14b(v)        (((uint32_t)(v) & 0x3FFF) << 6)      // System memory data line offset

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
    printk("00: 0x%08x\n",pmdma->SMaddr);      // 00
    printk("04: 0x%08x\n",pmdma->LMaddr);      // 04
    printk("08: 0x%08x\n",pmdma->BlkWidth);    // 08
    printk("0C: 0x%08x\n",pmdma->Control);     // 0C
    printk("10: 0x%08x\n",pmdma->CCA);         // 10
    printk("14: 0x%08x\n",pmdma->Status);      // 14
    printk("18: 0x%08x\n",pmdma->CCA2);        // 18   new feature from 2004 0507
    printk("1C: 0x%08x\n",pmdma->GRPC);        // 1C   Group execution control
    printk("20: 0x%08x\n",pmdma->GRPS);        // 20   Group sync control
    printk("24: 0x%08x\n",pmdma->ABFCtrl); 
}
#endif

#define USE_OFFSET
#ifdef SQUARE_BLOCK
static int rotate_block(uint8_t *out_addr, uint8_t *in_addr, int clockwise)
{
	uint32_t *r0, *r1;
	uint32_t *c0;
	int i, j;
	uint32_t chroma;
#ifdef USE_OFFSET
	int c0_offset;
#endif
	//memset(out_addr, 0, blk_w*blk_h*2);
	if (clockwise) {
		r0 = (uint32_t *)(in_addr);
		r1 = (uint32_t *)(in_addr + BLOCK_WIDTH2);
		for (i = 0; i < BLOCK_HEIGHT; i+=2) {
		#ifdef USE_OFFSET
            c0_offset = BLOCK_HEIGHT2-(i<<1)-4;
			c0 = (uint32_t *)(out_addr + c0_offset);
		#else
			//c0 = (uint32_t *)(out_addr + (BLOCK_HEIGHT<<1)-(i<<1)-4);
			c0 = (uint32_t *)(out_addr + (BLOCK_HEIGHT<<1)-i-4);
		#endif
			for (j = 0; j < BLOCK_WIDTH; j+=2) {
                uint32_t val0, val1, mask, tmp0, tmp1;
                val0 = *r0;
                val1 = *r1;
                mask = 0x00FF00FF;
                __asm 
                {
                    AND tmp0, mask, val0
                    AND tmp1, mask, val1
                    ADD tmp0, tmp0, tmp1
                    AND chroma, mask, tmp0, LSR #1
                }
                *c0 = ((val0<<16)&0xFF000000) | (val1&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += BLOCK_HEIGHT2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (BLOCK_HEIGHT>>1);
			#endif
				*c0 = (val0&0xFF000000) | ((val1>>16)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += BLOCK_HEIGHT2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (BLOCK_HEIGHT>>1);
			#endif
                r0++;
				r1++;
                /*
                    AND tmp0, m_h, val0, LSL #16
                    AND tmp1, m_l, val1
                    ORR tmp0, tmp0, tmp1
                    ORR tmp0, tmp0, chroma
                    STR tmp0, [c0], #32
                    AND tmp0, m_h, val0
                    AND tmp1, m_l, val1, LSR #16
                    ORR tmp0, tmp0, tmp1
                    ORR tmp0, tmp0, chroma
                    STR tmp0, [c0], #32
                }
                */
				//chroma = ((((*r0)&0x00FF00FF)+((*r1)&0x00FF00FF))>>1)&0x00FF00FF;
				/*
				*c0 = (((*r0)<<16)&0xFF000000) | ((*r1)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += BLOCK_HEIGHT2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (BLOCK_HEIGHT>>1);
			#endif
				*c0 = ((*r0)&0xFF000000) | (((*r1)>>16)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += BLOCK_HEIGHT2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (BLOCK_HEIGHT>>1);
			#endif
			    */
				//r0++;
				//r1++;
			}
			r0 += (BLOCK_WIDTH>>1);
			r1 += (BLOCK_WIDTH>>1);
            //break;
		}
	}
	else {
    #if 1
        r0 = (uint32_t *)in_addr;
		r1 = (uint32_t *)(in_addr + BLOCK_WIDTH2);
		for (i = 0; i < BLOCK_HEIGHT; i+=2) {
        #ifdef USE_OFFSET
			c0_offset = BLOCK_WIDTH2*BLOCK_HEIGHT - BLOCK_HEIGHT2 + (i<<1);
            c0 = (uint32_t *)(out_addr + c0_offset);
	    #else
			c0 = (uint32_t *)(out_addr + BLOCK_WIDTH2*BLOCK_HEIGHT - BLOCK_HEIGHT2 + (i<<1));
        #endif
			for (j = 0; j < BLOCK_WIDTH; j+=2) {
                uint32_t val0, val1, mask, tmp0, tmp1;
                val0 = *r0;
                val1 = *r1;
                mask = 0x00FF00FF;
                __asm 
                {
                    AND tmp0, mask, val0
                    AND tmp1, mask, val1
                    ADD tmp0, tmp0, tmp1
                    AND chroma, mask, tmp0, LSR #1
                }
                *c0 = ((val1<<16)&0xFF000000) | (val0&0x0000FF00) | chroma;
            #ifdef USE_OFFSET
				c0_offset -= BLOCK_HEIGHT2;
				c0 = (uint32_t *)(out_addr + c0_offset);
            #else
				c0 -= (BLOCK_HEIGHT>>1);
            #endif
				*c0 = (val1&0xFF000000) | ((val0>>16)&0x0000FF00) | chroma;
            #ifdef USE_OFFSET
				c0_offset -= BLOCK_HEIGHT2;
				c0 = (uint32_t *)(out_addr + c0_offset);
		    #else
				c0 -= (BLOCK_HEIGHT>>1);
            #endif
                /*
				chroma = ((((*r0)&0x00FF00FF) + ((*r1)&0x00FF00FF))>>1)&0x00FF00FF;
				*c0 = (((*r1)<<16)&0xFF000000) | ((*r0)&0x0000FF00) | chroma;
            #ifdef USE_OFFSET
				c0_offset -= BLOCK_HEIGHT2;
				c0 = (uint32_t *)(out_addr + c0_offset);
            #else
				c0 -= (BLOCK_HEIGHT>>1);
            #endif
				*c0 = ((*r1)&0xFF000000) | (((*r0)>>16)&0x0000FF00) | chroma;
            #ifdef USE_OFFSET
				c0_offset -= BLOCK_HEIGHT2;
				c0 = (uint32_t *)(out_addr + c0_offset);
		    #else
				c0 -= (BLOCK_HEIGHT>>1);
            #endif
                */
				r0++;
				r1++;
			}
			r0 += (BLOCK_WIDTH>>1);
			r1 += (BLOCK_WIDTH>>1);
		}
    #else
        r0 = (uint32_t *)out_addr;
        r1 = (uint32_t *)(out_addr + BLOCK_HEIGHT2);
        for (i = 0; i < BLOCK_WIDTH; i+=2) {
        #ifdef USE_OFFSET
        	c0_offset = BLOCK_WIDTH2-(i<<1)-4;
        	c1_offset = c0_offset + BLOCK_WIDTH2;
        	c0 = (uint32_t *)(in_addr + c0_offset);
        	c1 = (uint32_t *)(in_addr + c1_offset);
        #else
        	c0 = (uint32_t *)(in_addr + (BLOCK_WIDTH<<1)-(i<<1)-4);
        	c1 = c0 + (BLOCK_WIDTH>>1);
        #endif
        	for (j = 0; j < BLOCK_HEIGHT; j+=2) {
        		chroma = ((((*c0)&0x00FF00FF)+((*c1)&0x00FF00FF))>>1)&0x00FF00FF;
        		*r0 = ((*c1)&0xFF000000) | (((*c0)>>16)&0x0000FF00) | chroma;
        		*r1 = (((*c1)<<16)&0xFF000000) | ((*c0)&0x0000FF00) | chroma;
        	#ifdef USE_OFFSET
        		c0_offset += (BLOCK_WIDTH2<<1);
        		c1_offset += (BLOCK_WIDTH2<<1);
        		c0 = (uint32_t *)(in_addr + c0_offset);
        		c1 = (uint32_t *)(in_addr + c1_offset);
        	#else
        		c0 += BLOCK_WIDTH;
        		c1 += BLOCK_WIDTH;
        	#endif
        		r0++;
        		r1++;
        	}
        	r0 += (BLOCK_HEIGHT>>1);
        	r1 += (BLOCK_HEIGHT>>1);
        }
    #endif
	}
	return 0;
}

typedef struct
{
	int y;
	int x;
	int toggle;		// indicate which table will be used;
	int id;
} RT_mb;

static void fill_in_dma(RT_mb *m_in, rt_hw * posdhw)
{
    uint32_t *dma_cmd = (uint32_t *)(R90DBase(posdhw) + ((m_in->toggle==0)?DMACMD_OUT_0: DMACMD_OUT_1));
    if (0 == m_in->toggle) {
        dma_cmd[4 + 1] = 
        dma_cmd[8 + 1] = mDmaLocMemAddr14b (LOC_IN_0);
    }
    else {
        dma_cmd[4 + 1] = 
        dma_cmd[8 + 1] = mDmaLocMemAddr14b (LOC_IN_1);
    }    
    dma_cmd[4] = 
    dma_cmd[8] = posdhw->in_buf_base + ((m_in->y*posdhw->width + m_in->x)<<1);
    dma_cmd[4 + 2] =
    dma_cmd[8 + 2] =
        mDmaSysOff14b(posdhw->width*2/4+1 - BLOCK_WIDTH2/4) |          // SysOff
        mDmaSysWidth6b(BLOCK_WIDTH2/8);
    dma_cmd[4 + 3] =
        (1 << 26) |     //mask interrupt
        (1 << 23) |     //dma_en
        (1 << 21) |     //chain enable
        (0 << 20) |     //local->sys
        (0 << 18) |     //local: seq
        (1 << 16) |     //sys:seq
        (BLOCK_HEIGHT*BLOCK_WIDTH2/4);
    dma_cmd[4 + 3] =
        (0 << 26) |     //mask interrupt
        (1 << 23) |     //dma_en
        (0 << 21) |     //chain enable
        (0 << 20) |     //sys->local
        (0 << 18) |     //local: seq
        (1 << 16) |     //sys:seq
        (BLOCK_HEIGHT*BLOCK_WIDTH2/4);
}

static void fill_in_out_dma(RT_mb *m_in, RT_mb *m_out, rt_hw * posdhw)
{
    uint32_t *dma_cmd = (uint32_t *)(R90DBase(posdhw) + ((m_in->toggle==0)?DMACMD_OUT_0: DMACMD_OUT_1));
    if (posdhw->clockwise)
        dma_cmd[0] = posdhw->out_buf_base + ((m_out->x*posdhw->height-m_out->y+posdhw->height-BLOCK_HEIGHT)<<1);
    else
        dma_cmd[0] = posdhw->out_buf_base + (((posdhw->width-BLOCK_WIDTH-m_out->x)*posdhw->height+m_out->y)<<1);
    if (0 == m_in->toggle) 
        dma_cmd[0 + 1] = mDmaLocMemAddr14b (LOC_OUT_0);
    else
        dma_cmd[0 + 1] = mDmaLocMemAddr14b (LOC_OUT_1);
    dma_cmd[0 + 2] =
        mDmaSysOff14b(posdhw->height*2/4+1 - BLOCK_HEIGHT2/4) |          // SysOff
        mDmaSysWidth6b(BLOCK_HEIGHT2/8);
    dma_cmd[0 + 3] = 
            (1 << 26) |     //mask interrupt
            (1 << 23) |     //dma_en
            (1 << 21) |     //chain enable
            (1 << 20) |     //local->sys
            (0 << 18) |     //local: seq
            (1 << 16) |     //sys:seq
            (BLOCK_HEIGHT*BLOCK_WIDTH2/4);
    dma_cmd[4] = posdhw->in_buf_base + ((m_in->y*posdhw->width + m_in->x)<<1);
    if (0 == m_in->toggle)
        dma_cmd[4 + 1] = mDmaLocMemAddr14b (LOC_IN_0);
    else
        dma_cmd[4 + 1] = mDmaLocMemAddr14b (LOC_IN_1);
    dma_cmd[4 + 2] =
        mDmaSysOff14b(posdhw->width*2/4+1 - BLOCK_WIDTH2/4) |          // SysOff
        mDmaSysWidth6b(BLOCK_WIDTH2/8);
    dma_cmd[4 + 3] =
            (0 << 26) |     //mask interrupt
            (1 << 23) |     //dma_en
            (0 << 21) |     //chain enable
            (0 << 20) |     //sys->local
            (0 << 18) |     //local: seq
            (1 << 16) |     //sys:seq
            (BLOCK_HEIGHT*BLOCK_WIDTH2/4);
}

#else
static int rotate_block(uint8_t *out_addr, uint8_t *in_addr, int blk_w, int blk_h, int clockwise)
{
	uint32_t *r0, *r1;
	uint32_t *c0, *c1;
	int i, j;
	int blk_w2 = (blk_w<<1);
	int blk_h2 = (blk_h<<1);
    //int blk_w_half = (blk_w>>1);
    //int blk_h_half = (blk_h>>1);
	uint32_t chroma;// = 0x00800080;
#ifdef USE_OFFSET
	int c0_offset, c1_offset;
#endif
	//memset(out_addr, 0, blk_w*blk_h*2);
	if (clockwise) {
		r0 = (uint32_t *)(in_addr);
		r1 = (uint32_t *)(in_addr + blk_w2);
		for (i = 0; i < blk_h; i+=2) {
		#ifdef USE_OFFSET
            c0_offset = blk_h2-(i<<1)-4;
			c0 = (uint32_t *)(out_addr + c0_offset);
		#else
			//c0 = (uint32_t *)(out_addr + (blk_h<<1)-(i<<1)-4);
			c0 = (uint32_t *)(out_addr + (blk_h<<1)-i-4);
		#endif
			for (j = 0; j < blk_w; j+=2) {
				chroma = ((((*r0)&0x00FF00FF)+((*r1)&0x00FF00FF))>>1)&0x00FF00FF;
				*c0 = (((*r0)<<16)&0xFF000000) | ((*r1)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += blk_h2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (blk_h>>1);
			#endif
				*c0 = ((*r0)&0xFF000000) | (((*r1)>>16)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
                c0_offset += blk_h2;
                c0 = (uint32_t *)(out_addr + c0_offset);
			#else
				c0 += (blk_h>>1);
			#endif
				r0++;
				r1++;
			}
			r0 += (blk_w>>1);
			r1 += (blk_w>>1);
            //break;
		}
	}
	else {
        r0 = (uint32_t *)out_addr;
		r1 = (uint32_t *)(out_addr + blk_h2);
		for (i = 0; i < blk_w; i+=2) {
		#ifdef USE_OFFSET
			c0_offset = blk_w2-(i<<1)-4;
			c1_offset = c0_offset + blk_w2;
			c0 = (uint32_t *)(in_addr + c0_offset);
			c1 = (uint32_t *)(in_addr + c1_offset);
		#else
			c0 = (uint32_t *)(in_addr + (blk_w<<1)-(i<<1)-4);
			c1 = c0 + (blk_w>>1);
		#endif
			for (j = 0; j < blk_h; j+=2) {
				chroma = ((((*c0)&0x00FF00FF)+((*c1)&0x00FF00FF))>>1)&0x00FF00FF;
				*r0 = ((*c1)&0xFF000000) | (((*c0)>>16)&0x0000FF00) | chroma;
				*r1 = (((*c1)<<16)&0xFF000000) | ((*c0)&0x0000FF00) | chroma;
			#ifdef USE_OFFSET
				c0_offset += (blk_w2<<1);
				c1_offset += (blk_w2<<1);
				c0 = (uint32_t *)(in_addr + c0_offset);
				c1 = (uint32_t *)(in_addr + c1_offset);
			#else
				c0 += blk_w;
				c1 += blk_w;
			#endif
				r0++;
				r1++;
			}
			r0 += (blk_h>>1);
			r1 += (blk_h>>1);
		}
	}
	return 0;
}

typedef struct
{
	int y;
	int x;
	int toggle;		// indicate which table will be used;
	int blk_w;
	int id;
} RT_mb;

static void fill_in_dma(RT_mb *m_in, rt_hw * posdhw)
{
    uint32_t *dma_cmd = (uint32_t *)(R90DBase(posdhw) + ((m_in->toggle==0)?DMACMD_OUT_0: DMACMD_OUT_1));
    m_in->blk_w = posdhw->width - m_in->x;
    if (m_in->blk_w > BLOCK_WIDTH)
        m_in->blk_w = BLOCK_WIDTH;

    if (0 == m_in->toggle) {
        dma_cmd[4 + 1] = 
        dma_cmd[8 + 1] = mDmaLocMemAddr14b (LOC_IN_0);
    }
    else {
        dma_cmd[4 + 1] = 
        dma_cmd[8 + 1] = mDmaLocMemAddr14b (LOC_IN_1);
    }    
    dma_cmd[4] = 
    dma_cmd[8] = posdhw->in_buf_base + ((m_in->y*posdhw->width + m_in->x)<<1);
    dma_cmd[4 + 2] =
    dma_cmd[8 + 2] =
        mDmaSysOff14b(posdhw->width*2/4+1 - m_in->blk_w*2/4) |          // SysOff
        mDmaSysWidth6b(2*m_in->blk_w/8);
    dma_cmd[4 + 3] =
        (1 << 26) |     //mask interrupt
        (1 << 23) |     //dma_en
        (1 << 21) |     //chain enable
        (0 << 20) |     //local->sys
        (0 << 18) |     //local: seq
        (1 << 16) |     //sys:seq
        (BLOCK_HEIGHT*m_in->blk_w*2/4);
    dma_cmd[4 + 3] =
        (0 << 26) |     //mask interrupt
        (1 << 23) |     //dma_en
        (0 << 21) |     //chain enable
        (0 << 20) |     //sys->local
        (0 << 18) |     //local: seq
        (1 << 16) |     //sys:seq
        (BLOCK_HEIGHT*m_in->blk_w*2/4);
}

static void fill_in_out_dma(RT_mb *m_in, RT_mb *m_out, rt_hw * posdhw)
{
    uint32_t *dma_cmd = (uint32_t *)(R90DBase(posdhw) + ((m_in->toggle==0)?DMACMD_OUT_0: DMACMD_OUT_1));
    m_in->blk_w = posdhw->width - m_in->x;
    if (m_in->blk_w > BLOCK_WIDTH)
        m_in->blk_w = BLOCK_WIDTH;
    if (posdhw->clockwise)
        dma_cmd[0] = posdhw->out_buf_base + ((m_out->x*posdhw->height-m_out->y+posdhw->height-BLOCK_HEIGHT)<<1);
    else
        dma_cmd[0] = posdhw->out_buf_base + (((posdhw->width-m_out->blk_w-m_out->x)*posdhw->height+m_out->y)<<1);
    if (0 == m_in->toggle) 
        dma_cmd[0 + 1] = mDmaLocMemAddr14b (LOC_OUT_0);
    else
        dma_cmd[0 + 1] = mDmaLocMemAddr14b (LOC_OUT_1);
    dma_cmd[0 + 2] =
        mDmaSysOff14b(posdhw->height*2/4+1 - BLOCK_HEIGHT*2/4) |          // SysOff
        mDmaSysWidth6b(2*BLOCK_HEIGHT/8);
    dma_cmd[0 + 3] = 
            (1 << 26) |     //mask interrupt
            (1 << 23) |     //dma_en
            (1 << 21) |     //chain enable
            (1 << 20) |     //local->sys
            (0 << 18) |     //local: seq
            (1 << 16) |     //sys:seq
            (BLOCK_HEIGHT*m_out->blk_w*2/4);
    dma_cmd[4] = posdhw->in_buf_base + ((m_in->y*posdhw->width + m_in->x)<<1);
    if (0 == m_in->toggle)
        dma_cmd[4 + 1] = mDmaLocMemAddr14b (LOC_IN_0);
    else
        dma_cmd[4 + 1] = mDmaLocMemAddr14b (LOC_IN_1);
    dma_cmd[4 + 2] =
        mDmaSysOff14b(posdhw->width*2/4+1 - m_in->blk_w*2/4) |          // SysOff
        mDmaSysWidth6b(2*m_in->blk_w/8);
    dma_cmd[4 + 3] =
            (0 << 26) |     //mask interrupt
            (1 << 23) |     //dma_en
            (0 << 21) |     //chain enable
            (0 << 20) |     //sys->local
            (0 << 18) |     //local: seq
            (1 << 16) |     //sys:seq
            (BLOCK_HEIGHT*m_in->blk_w*2/4);
}
#endif
// 1. dma in
// 2. rotation
// 3. dma out
void rotation_start(rt_hw * posdhw)  
{
    volatile MDMA *pmdma = (volatile MDMA *)(R90DBase(posdhw) + DMA_OFF);
    RT_mb mb[3];
    RT_mb *m_in = &mb[0], *m_rt = &mb[1], *m_out = &mb[2], *m_tmp;
    int pipe = 0;

/*
    mb[0].id = 0;
    mb[1].id = 1;
    mb[2].id = 2;

    memset((void *)(R90DBase(posdhw)+LOC_OUT_0), 0, BLOCK_HEIGHT*BLOCK_WIDTH*2);
    memset((void *)(R90DBase(posdhw)+LOC_OUT_1), 0, BLOCK_HEIGHT*BLOCK_WIDTH*2);
*/
    //dma_init_rt(R90DBase(posdhw), posdhw->width, posdhw->height);
    m_out->y = m_rt->y = m_in->y = 0;
    m_out->x = m_rt->x = m_in->x = 0;
#ifndef SQUARE_BLOCK
    m_out->blk_w = m_rt->blk_w = m_in->blk_w = BLOCK_WIDTH;
#endif
    m_out->toggle = m_rt->toggle = m_in->toggle = 0;
    pipe = BIT_IN;
    while (1) {
        if (pipe & (BIT_IN|BIT_OUT)) {
            if (0 == (pipe & BIT_OUT)) {
                fill_in_dma(m_in, posdhw);
                pmdma->CCA = ((0==m_in->toggle)? DMACMD_IN_0: DMACMD_IN_1) | 0x02;
                pmdma->Control = (0x04A00000); //start DMA from local memory
            }
            else {
                fill_in_out_dma(m_in, m_out, posdhw);
                if (m_in->toggle != m_out->toggle) {
                    break;
                }
                pmdma->CCA = ((0==m_out->toggle)? DMACMD_OUT_0: DMACMD_OUT_1) | 0x02;
                pmdma->Control = (0x04A00000); //start DMA from local memory
                //cnt++;
                //if (posdhw->stop_mb_cnt > 0 && cnt >= posdhw->stop_mb_cnt)
                //    break;
            }
        }
        if (pipe & BIT_RT) {
        #ifdef SQUARE_BLOCK
            if (0 == m_rt->toggle) {
                rotate_block((uint8_t*)(R90DBase(posdhw)+LOC_OUT_0), 
                    (uint8_t*)(R90DBase(posdhw)+LOC_IN_0), posdhw->clockwise);
                //memcpy((void *)(R90DBase(posdhw)+LOC_OUT_0), (void *)(R90DBase(posdhw)+LOC_IN_0), BLOCK_HEIGHT*m_rt->blk_w*2);
            }
            else {
                rotate_block((uint8_t*)(R90DBase(posdhw)+LOC_OUT_1), 
                    (uint8_t*)(R90DBase(posdhw)+LOC_IN_1), posdhw->clockwise);
                //memcpy((void *)(R90DBase(posdhw)+LOC_OUT_1), (void *)(R90DBase(posdhw)+LOC_IN_1), BLOCK_HEIGHT*m_rt->blk_w*2);
            }
        #else
            if (0 == m_rt->toggle) {
                rotate_block((uint8_t*)(R90DBase(posdhw)+LOC_OUT_0), 
                    (uint8_t*)(R90DBase(posdhw)+LOC_IN_0), m_rt->blk_w, BLOCK_HEIGHT, posdhw->clockwise);
                //memcpy((void *)(R90DBase(posdhw)+LOC_OUT_0), (void *)(R90DBase(posdhw)+LOC_IN_0), BLOCK_HEIGHT*m_rt->blk_w*2);
            }
            else {
                rotate_block((uint8_t*)(R90DBase(posdhw)+LOC_OUT_1), 
                    (uint8_t*)(R90DBase(posdhw)+LOC_IN_1), m_rt->blk_w, BLOCK_HEIGHT, posdhw->clockwise);
                //memcpy((void *)(R90DBase(posdhw)+LOC_OUT_1), (void *)(R90DBase(posdhw)+LOC_IN_1), BLOCK_HEIGHT*m_rt->blk_w*2);
            }
        #endif
            pipe &= ~BIT_RT;
            pipe |= BIT_RT_DONE;
        }
        if (pipe & (BIT_IN|BIT_OUT)) {
            while(!(pmdma->Status & 0x1))
                ;
            if (pipe & BIT_IN)
                pipe |= BIT_RT;
            pipe &= ~(BIT_IN|BIT_OUT);
        }
        if (pipe & BIT_RT_DONE) {
            pipe &= ~BIT_RT_DONE;
            pipe |= BIT_OUT;
        }

        m_tmp = m_out;
        m_out = m_rt;
        m_rt = m_in;
        m_in = m_tmp;
        m_in->y = m_rt->y;
    #ifdef SQUARE_BLOCK
        m_in->x = m_rt->x + BLOCK_WIDTH;
    #else
        m_in->x = m_rt->x + m_rt->blk_w;
    #endif
        m_in->toggle = m_rt->toggle^0x01;
        if (m_in->x >= posdhw->width) {
            m_in->y += BLOCK_HEIGHT;
            m_in->x = 0;
        }
        /*  // set @ fill dma cmd
        m_in->blk_w = posdhw->width - m_in->x;
        if (m_in->blk_w > BLOCK_WIDTH)
            m_in->blk_w = BLOCK_WIDTH;
        */
        if (m_in->y < posdhw->height)
            pipe |= BIT_IN;
        if (0 == pipe)
            break;
    }
    return;
}


#endif
