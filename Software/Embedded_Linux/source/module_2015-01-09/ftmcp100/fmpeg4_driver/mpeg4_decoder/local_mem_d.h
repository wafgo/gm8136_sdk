#ifndef __LOCAL_MEM_D_H
	#define __LOCAL_MEM_D_H
#ifdef LINUX
	#include "Mp4Vdec.h"
	#include "local_mem.h"
	#include "../common/mp4.h"
#else
	#include "Mp4Vdec.h"
	#include "local_mem.h"
	#include "mp4.h"
#endif
    #define RGB_PIXEL_SIZE_1 1
    #define RGB_PIXEL_SIZE_2 2
    #define RGB_PIXEL_SIZE_4 4
	//#define RGB_PIXEL_SIZE(pixel_size) (pixel_size)	 
       
/****************  bank 0	*****************************************/
	// 000 ~ 300, Leng = (2 * 64) * 6 (blocks)		// must 0x100 bundary
	#define QCOEFF_OFF_0		(LOCAL_BASE + 0x000)
	// 300 ~ 600, Leng = (2 * 64) * 6 (blocks)		// must 0x100 bundary
	#define QCOEFF_OFF_1		(LOCAL_BASE + 0x300)
	// 600 ~ 900, Leng = (2 * 64) * 6 (blocks)		// must 0x100 bundary
	#define QCOEFF_OFF_2		(LOCAL_BASE + 0x600)

	// 000 ~ 100, Leng = (4 * 4) * 16(structures)
	#define DMA_CMD_N_OFF	(QCOEFF_OFF_0)

	// 900 ~ B00, Leng = (4 * 4) * 32(structures)
	#define DMA_DEC_CMD_OFF_0	(LOCAL_BASE + 0x900)
	// B00 ~ D00, Leng = (4 * 4) * 32(structures)
	#define DMA_DEC_CMD_OFF_1	(LOCAL_BASE + 0xB00)
	// D00 ~ E00,		VLC/VLD auto buffer,	define in local_mem.h
	// E00 ~ 1000, 	ACDC predictor buffer,	define in local_mem.h
	// 1000 ~ 1400,	PMV buffer,			define in local_mem.h
	// 1400 ~ 1600, Leng = (4) * 64(commands count) 		// must 0x200 bundary
	#define ME_CMD_Q_OFF		(LOCAL_BASE + 0x1400)

	// 1600 ~ 1800,	ME cmd chain,			define in local_mem_e.h, used by encoder
	// 1800 ~ 1A00,	DMA cmd chain,		define in local_mem_e.h, used by encoder
/****************  bank 1	(2K bytes)*****************************************/
	// ping-pong interpolation buffer: 4000 ~ 4300
	#define INTER_Y_OFF_0		(LOCAL_BASE + 0x4000)	//	000 -- 100, Leng = 4 * SIZE_Y
	#define INTER_U_OFF_0 		(INTER_Y_OFF_0 + SIZE_Y)//	100 -- 140, Leng = SIZE_U
	#define INTER_V_OFF_0 		(INTER_U_OFF_0 + SIZE_U)//	140 -- 180, Leng = SIZE_V

	#define INTER_Y_OFF_1		(INTER_V_OFF_0 + SIZE_V)//	180 -- 280, Leng = 4 * SIZE_Y
	#define INTER_U_OFF_1 		(INTER_Y_OFF_1 + SIZE_Y)//	280 -- 2C0, Leng = SIZE_U
	#define INTER_V_OFF_1 		(INTER_U_OFF_1 + SIZE_U)//	2C0 -- 300, Leng = SIZE_V
	#define INTER_Y_STEP		(SIZE_Y + SIZE_U + SIZE_V)

  #ifdef ENABLE_DEBLOCKING
    // ping-pong RGB buffer: 4300 ~ 4900 or 4300 ~ 4F00
	//other: 300 -- 700, Leng = 256 pixel * 2 B * 2 (ping-pong)
	//888:   300 -- B00, Leng = 256 pixel * 4 B * 2 (ping-pong)
	// for Deblocking, need 1.5 time of above
	//	==> 		//other: 300 -- 900, Leng = 256 pixel * 2 B * 2 *1.5 (ping-pong)
					//888:   300 -- F00, Leng = 256 pixel * 4 B * 2 *1.5(ping-pong)
	#define BUFFER_RGB_OFF_0(pixel_size)			(LOCAL_BASE + 0x4300)
	#define BUFFER_RGB_TOP_OFF_0(pixel_size)		(BUFFER_RGB_OFF_0(pixel_size))
	#define BUFFER_RGB_MID_OFF_0(pixel_size)		(BUFFER_RGB_OFF_0(pixel_size) + SIZE_Y * pixel_size /2)
	#define BUFFER_RGB_BOT_OFF_0(pixel_size)		(BUFFER_RGB_OFF_0(pixel_size) + SIZE_Y * pixel_size)
	#define BUFFER_RGB_OFF_1(pixel_size)			(BUFFER_RGB_OFF_0(pixel_size) + SIZE_Y * pixel_size * 3/2)
	#define BUFFER_RGB_TOP_OFF_1(pixel_size)		(BUFFER_RGB_OFF_1(pixel_size))
	#define BUFFER_RGB_MID_OFF_1(pixel_size)		(BUFFER_RGB_OFF_1(pixel_size) + SIZE_Y * pixel_size /2)
	#define BUFFER_RGB_BOT_OFF_1(pixel_size)		(BUFFER_RGB_OFF_1(pixel_size) + SIZE_Y * pixel_size)  
  #else // #ifdef ENABLE_DEBLOCKING
	// ping-pong RGB buffer: 4300 ~ 4700 or 4300 ~ 4B00
	#define BUFFER_RGB_OFF_0(pixel_size)	(LOCAL_BASE + 0x4300)	//other: 300 -- 700, Leng = 256 pixel * 2 B * 2 (ping-pong)
												//888:   300 -- B00, Leng = 256 pixel * 4 B * 2 (ping-pong)
	#define BUFFER_RGB_OFF_1(pixel_size)	(BUFFER_RGB_OFF_0(pixel_size) + SIZE_Y * pixel_size)
  #endif // #ifdef ENABLE_DEBLOCKING

/****************  bank 2/3	*****************************************/
	// 8000 ~ 8C00, Leng = C00h = (8 * 8) (bytes/block) * (8 * 6)(blocks)
	#define REF_Y_OFF_0		(LOCAL_BASE + 0x8000)
	#define REF_Y0_OFF_0	REF_Y_OFF_0
	#define REF_Y1_OFF_0	(REF_Y0_OFF_0 + 2 * PIXEL_U)
	#define REF_Y2_OFF_0	(REF_Y0_OFF_0 + 16 * SIZE_U)
	#define REF_Y3_OFF_0	(REF_Y2_OFF_0 + 2 * PIXEL_U)
	#define REF_U_OFF_0		(REF_Y0_OFF_0 + 32 * SIZE_U)
	#define REF_V_OFF_0		(REF_U_OFF_0 + 2 * PIXEL_U)

	#define REF_Y_OFF_1		(REF_Y_OFF_0 + 4 * PIXEL_U)
	#define REF_Y0_OFF_1	REF_Y_OFF_1
	#define REF_Y1_OFF_1	(REF_Y0_OFF_1 + 2 * PIXEL_U)
	#define REF_Y2_OFF_1	(REF_Y0_OFF_1 + 16 * SIZE_U)
	#define REF_Y3_OFF_1	(REF_Y2_OFF_1 + 2 * PIXEL_U)
	#define REF_U_OFF_1		(REF_Y0_OFF_1 + 32 * SIZE_U)
	#define REF_V_OFF_1		(REF_U_OFF_1 + 2 * PIXEL_U)

	// 8C00 ~
	#define TABLE_OUTPUT_OFF	(LOCAL_BASE + 0x8C00)
/****************  bank 4	*****************************************/
	// 7C00 ~ 7D00,	intra quant table,		define in local_mem.h
	// 7D00 ~ 7F00,	inter quant table,		define in local_mem.h

#ifdef ENABLE_DEBLOCKING	
/****************  bank 5	*****************************************/
	// A000
	#define DEBK_LD_OFF_0		(LOCAL_BASE + 0xA000)
	#define DEBK_ST_OFF_0		(LOCAL_BASE + 0xA0C0)
	#define DEBK_TOP_Y_OFF_0	(LOCAL_BASE + 0xA000)
	#define DEBK_MID_Y_OFF_0	(LOCAL_BASE + 0xA180)
	#define DEBK_BOT_Y_OFF_0	(LOCAL_BASE + 0xA0C0)
	#define DEBK_TOP_U_OFF_0	(LOCAL_BASE + 0xA080)
	#define DEBK_MID_U_OFF_0	(LOCAL_BASE + 0xA400)
	#define DEBK_BOT_U_OFF_0	(LOCAL_BASE + 0xA140)
	#define DEBK_TOP_V_OFF_0	(LOCAL_BASE + 0xA0A0)
	#define DEBK_MID_V_OFF_0	(LOCAL_BASE + 0xA420)
	#define DEBK_BOT_V_OFF_0	(LOCAL_BASE + 0xA160)

	#define DEBK_LD_OFF_1		(LOCAL_BASE + 0xA200)
	#define DEBK_ST_OFF_1		(LOCAL_BASE + 0xA2C0)
	#define DEBK_TOP_Y_OFF_1	(LOCAL_BASE + 0xA200)
	#define DEBK_MID_Y_OFF_1	(LOCAL_BASE + 0xA380)
	#define DEBK_BOT_Y_OFF_1	(LOCAL_BASE + 0xA2C0)
	#define DEBK_TOP_U_OFF_1	(LOCAL_BASE + 0xA280)
	#define DEBK_MID_U_OFF_1	(LOCAL_BASE + 0xA440)
	#define DEBK_BOT_U_OFF_1	(LOCAL_BASE + 0xA340)
	#define DEBK_TOP_V_OFF_1	(LOCAL_BASE + 0xA2A0)
	#define DEBK_MID_V_OFF_1	(LOCAL_BASE + 0xA460)
	#define DEBK_BOT_V_OFF_1	(LOCAL_BASE + 0xA360)
#endif // #ifdef ENABLE_DEBLOCKING

#endif
