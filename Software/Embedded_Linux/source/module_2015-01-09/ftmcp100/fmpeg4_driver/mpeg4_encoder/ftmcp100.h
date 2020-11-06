/**
 *  @file ftmcp100.h
 *  @brief This file consists of various kind of definitions for ftmcp100 media 
 *         coprocessor architecture.
 *
 */
#ifndef FTMCP100_H
#define FTMCP100_H
#ifdef LINUX
#include "global_e.h"
#include "Mp4Venc.h"
#include "../common/mp4.h"
#else
#include "global_e.h"
#include "mp4venc.h"
#include "mp4.h"
#endif

//---------------------------------------------------------------------------

#define MAX_XDIM			1920
#define MAX_YDIM			1080
  
 //---------------------------------------------------------------------------
// mainly used for RTL debug port output and marker output
//#define ENABLE_RTL_MONITOR


#define VPE 		0x90180000
// the mPOLL_MARKER_* was used to set a marker for verilog
// simulation debug and waveform observation purpose by 
// setting QCR_2 register which is not used in MPEG4 firmware
#if defined(FPGA) || !defined(ENABLE_RTL_MONITOR)
	#define mPOLL_MARKER_S(ptMP4)
	#define mPOLL_MARKER_E(ptMP4)
	#define mPOLL_MC_DONE_MARKER_START(ptMP4)
	#define mPOLL_MC_DONE_MARKER_END(ptMP4)
	#define mPOLL_ME_DONE_MARKER_START(ptMP4)
	#define mPOLL_ME_DONE_MARKER_END(ptMP4)
	#define mPOLL_PMV_DONE_MARKER_START(ptMP4)
	#define mPOLL_PMV_DONE_MARKER_END(ptMP4)
	#define mPOLL_MARKER1_START(ptMP4)
	#define mPOLL_MARKER1_END(ptMP4)
	#define mPOLL_MECP_DONE_MARKER_START(ptMP4)
	#define mPOLL_MECP_DONE_MARKER_END(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_START_0(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_END_0(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_START_1(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_END_1(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_START_2(ptMP4)
	#define mPOLL_MECMD_SET_MARKER_END_2(ptMP4)
	#define mPOLL_VLC_DONE_MARKER_START(ptMP4)
	#define mPOLL_VLC_DONE_MARKER_END(ptMP4)
	#define mPOLL_MECMD_SET_MARKER(ptMP4,v)
	#define mPOLL_MECMD_SET_MARKER_2(ptMP4,v)
	#define RTL_DEBUG_OUT(v)	
#else
//	#if defined(ENABLE_RTL_MONITOR)
		#define RTL_DEBUG_OUT(v) *(unsigned int*)VPE=v;
		#define mPOLL_MARKER_S(ptMP4)	{ptMP4->QCR2 = 0x00;}
		#define mPOLL_MARKER_E(ptMP4)	{ptMP4->QCR2 = 0x01;}
		#define mPOLL_MC_DONE_MARKER_START(ptMP4)	{ptMP4->QCR2 = 0x02;}
		#define mPOLL_MC_DONE_MARKER_END(ptMP4)	{ptMP4->QCR2 = 0x03;}
		#define mPOLL_ME_DONE_MARKER_START(ptMP4)	{ptMP4->QCR2 = 0x04;}
		#define mPOLL_ME_DONE_MARKER_END(ptMP4)	{ptMP4->QCR2 = 0x05;}
		#define mPOLL_PMV_DONE_MARKER_START(ptMP4)	{ptMP4->QCR2 = 0x06;}
		#define mPOLL_PMV_DONE_MARKER_END(ptMP4)	{ptMP4->QCR2 = 0x07;}
		#define mPOLL_MARKER1_START(ptMP4)	{ptMP4->QCR2 = 0x08;}
		#define mPOLL_MARKER1_END(ptMP4)	{ptMP4->QCR2 = 0x09;}
		#define mPOLL_MECP_DONE_MARKER_START(ptMP4)	{ptMP4->QCR2 = 0x0a;}
		#define mPOLL_MECP_DONE_MARKER_END(ptMP4)	{ptMP4->QCR2 = 0x0b;}
		#define mPOLL_MECMD_SET_MARKER_START_0(ptMP4)	{ptMP4->QCR2 = 0xc0;}
		#define mPOLL_MECMD_SET_MARKER_END_0(ptMP4)	{ptMP4->QCR2 = 0xd0;}
		#define mPOLL_MECMD_SET_MARKER_START_1(ptMP4)	{ptMP4->QCR2 = 0xc1;}
		#define mPOLL_MECMD_SET_MARKER_END_1(ptMP4)	{ptMP4->QCR2 = 0xd1;}
		#define mPOLL_MECMD_SET_MARKER_START_2(ptMP4)	{ptMP4->QCR2 = 0xc2;}
		#define mPOLL_MECMD_SET_MARKER_END_2(ptMP4)	{ptMP4->QCR2 = 0xd2;}
		#define mPOLL_VLC_DONE_MARKER_START(ptMP4)	{ptMP4->QCR2 = 0x0e;}
		#define mPOLL_VLC_DONE_MARKER_END(ptMP4)	{ptMP4->QCR2 = 0x0f;}
		//#define mPOLL_MECMD_SET_MARKER(ptMP4,v)	    {ptMP4->QCR2 = v;}
		#define mPOLL_MECMD_SET_MARKER(ptMP4,v)
		#define mPOLL_MECMD_SET_MARKER_2(ptMP4,v)    {ptMP4->QCR2 = v;}
		//#define mPOLL_MECMD_SET_MARKER_2(ptMP4,v)
//#endif
#endif


//---------------------------------------------------------------------------
#endif
