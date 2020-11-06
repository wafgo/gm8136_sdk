//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINBUS.C
// Description 		:  Host interface Protocol handling
// Ref. Docment		: 
// Revision History 	:

//--------------------------------------------------------------------------------------------------------------------------
// Note for dma operation
//--------------------------------------------------------------------------------------------------------------------------
// GET_MDIN_DREQ() is GPIO(IN)/IRQ pin of CPU. This is connected to DREQ_N of MDIN380.

//--------------------------------------------------------------------------------------------------------------------------
// You must make drive functions for BUS read & BUS write.
//--------------------------------------------------------------------------------------------------------------------------
// static void MDINBUS_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);
// static void MDINBUS_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);

//--------------------------------------------------------------------------------------------------------------------------
// If you want to use dma-function, you must make drive functions for DMA read & DMA write. 
//--------------------------------------------------------------------------------------------------------------------------
// MDIN_ERROR_t MDINBUS_DMAWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
// MDIN_ERROR_t MDINBUS_DMARead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include	"mdin3xx.h"
#include	"..\common\board.h"		// cpu dependent for bus & dma operation

#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_BUS_HIF)
// ----------------------------------------------------------------------
// Struct/Union Types and define
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static WORD PageID = 0;

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------
static void MDINBUS_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);
static void MDINBUS_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes);

MDIN_ERROR_t MDINBUS_DMAWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINBUS_DMARead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Global functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
static void MDINBUS_SetPage(BYTE nID, WORD page)
{
	MDINI2C_SetPageID(page);	// set pageID to I2C-IF
	if (page==PageID) return;	PageID = page;
	MDINBUS_Write(nID, 0x800, (PBYTE)&page, 2);	// write page
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHOST_BUSWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	MDINBUS_SetPage(MDIN_HOST_ID, 0x0000);	// write host page
	MDINBUS_Write(MDIN_HOST_ID, rAddr*2, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHOST_BUSRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	MDINBUS_SetPage(MDIN_HOST_ID, 0x0000);	// write host page
	MDINBUS_Read(MDIN_HOST_ID, rAddr*2, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void LOCAL_BUSWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	MDINBUS_SetPage(MDIN_LOCAL_ID, 0x0101);	// write local page
	MDINBUS_Write(MDIN_LOCAL_ID, rAddr*2, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void LOCAL_BUSRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD RegOEN;

	if		(rAddr>=0x030&&rAddr<0x036)	RegOEN = 0x04;	// mfc-size
	else if (rAddr>=0x043&&rAddr<0x045)	RegOEN = 0x09;	// out-ptrn
	else if (rAddr>=0x062&&rAddr<0x083)	RegOEN = 0x09;	// enhance
	else if (rAddr>=0x088&&rAddr<0x092)	RegOEN = 0x09;	// out-sync
	else if (rAddr>=0x094&&rAddr<0x097)	RegOEN = 0x09;	// out-sync
	else if (rAddr>=0x09a&&rAddr<0x09c)	RegOEN = 0x09;	// bg-color
	else if (rAddr>=0x0a0&&rAddr<0x0d0)	RegOEN = 0x09;	// out-ctrl
	else if (              rAddr<0x100)	RegOEN = 0x01;	// in-sync
	else if (rAddr>=0x100&&rAddr<0x140)	RegOEN = 0x05;	// main-fc
	else if (rAddr>=0x140&&rAddr<0x1a0)	RegOEN = 0x07;	// aux
	else if (rAddr>=0x1a0&&rAddr<0x1c0)	RegOEN = 0x03;	// arbiter
	else if (rAddr>=0x1c0&&rAddr<0x1e0)	RegOEN = 0x02;	// fc-mc
	else if (rAddr>=0x1e0&&rAddr<0x1f8)	RegOEN = 0x08;	// encoder
	else if (rAddr>=0x1f8&&rAddr<0x200)	RegOEN = 0x0a;	// audio
	else if (rAddr>=0x200&&rAddr<0x280)	RegOEN = 0x04;	// ipc
	else if (rAddr>=0x2a0&&rAddr<0x300)	RegOEN = 0x07;	// aux-osd
	else if (rAddr>=0x300&&rAddr<0x380)	RegOEN = 0x06;	// osd
	else if (rAddr>=0x3c0&&rAddr<0x3f8)	RegOEN = 0x09;	// enhance
	else								RegOEN = 0x00;	// host state

	LOCAL_BUSWrite(0x3ff, (PBYTE)&RegOEN, 2);	// write reg_oen
	MDINBUS_Read(MDIN_LOCAL_ID, rAddr*2, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MHDMI_GetWriteDone(void)
{
	WORD rVal = 0, count = 100;

	while (count&&(rVal==0)) {
		MDINBUS_Read(MDIN_HOST_ID, 0x027*2, (PBYTE)&rVal, 2);
		rVal &= 0x04;	count--;
	}
	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHDMI_BUSWrite(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD i, waddr, wdata, wctrl, err = 0;

	MDINBUS_SetPage(MDIN_HOST_ID, 0x0000);	// write host page

#if CPU_ACCESS_BUS_NBYTE == 1 				// bus width = 16bit, ignore endian
	for (i=0; i<bytes/2; i++) {
		waddr = rAddr+i*2;	wdata = LOWORD(((PWORD)pBuff)[i]);
							MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
							MDINBUS_Write(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);
		wctrl = 0x0001;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0000;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);

		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
	}
#else										// bus width = 8bit, ignore endian
	for (i=0; i<bytes/2; i++) {
		waddr = rAddr+i*2;	wdata = LOBYTE(((PWORD)pBuff)[i]);
							MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
							MDINBUS_Write(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);
		wctrl = 0x0001;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0000;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag

		waddr = waddr+1;	wdata = HIBYTE(((PWORD)pBuff)[i]);
							MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
							MDINBUS_Write(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);
		wctrl = 0x0001;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0000;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
	}
#endif

	if (err==MDIN_TIMEOUT_ERROR) mdinERR = 3;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHDMI_BUSRead(WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD i, waddr, wdata, wctrl, err = 0;

	MDINBUS_SetPage(MDIN_HOST_ID, 0x0000);	// write host page

#if CPU_ACCESS_BUS_NBYTE == 1 				// bus width = 16bit, ignore endian
	for (i=0; i<bytes/2; i++) {
		waddr = rAddr+i*2;	MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
		wctrl = 0x0003;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0002;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);

		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
		MDINBUS_Read(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);	((PWORD)pBuff)[i]  = wdata<<0;
	}
#else										// bus width = 8bit, ignore endian
	for (i=0; i<bytes/2; i++) {
		waddr = rAddr+i*2;	MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
		wctrl = 0x0003;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0002;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
		MDINBUS_Read(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);	((PWORD)pBuff)[i]  = wdata<<0;

		waddr = waddr+1;	MDINBUS_Write(MDIN_HOST_ID, 0x025*2, (PBYTE)&waddr, 2);
		wctrl = 0x0003;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		wctrl = 0x0002;		MDINBUS_Write(MDIN_HOST_ID, 0x027*2, (PBYTE)&wctrl, 2);
		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
		MDINBUS_Read(MDIN_HOST_ID, 0x026*2, (PBYTE)&wdata, 2);	((PWORD)pBuff)[i] |= wdata<<8;
	}
#endif

	if (err==MDIN_TIMEOUT_ERROR) mdinERR = 4;
}

//--------------------------------------------------------------------------------------------------------------------------
static void SDRAM_BUSWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD row, len, idx, unit = 4096;	// fix for MDIN380

	while (bytes>0) {
		row = ADDR2ROW(rAddr, unit);	// get row
		idx = ADDR2COL(rAddr, unit);	// get col
		len = MIN((unit/2)-(rAddr%(unit/2)), bytes);

		MDINBUS_RegWrite(MDIN_HOST_ID, 0x003, row);	// host access
		MDINBUS_SetPage(MDIN_SDRAM_ID, 0x0303);	// write sdram page
		MDINBUS_Write(MDIN_SDRAM_ID, idx, pBuff, len);
		bytes-=len; rAddr+=len; pBuff+=len;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
static void SDRAM_BUSRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD row, len, idx, unit = 4096;	// fix for MDIN380

	while (bytes>0) {
		row = ADDR2ROW(rAddr, unit);	// get row
		idx = ADDR2COL(rAddr, unit);	// get col
		len = MIN((unit/2)-(rAddr%(unit/2)), bytes);

		MDINBUS_RegWrite(MDIN_HOST_ID, 0x003, row);	// host access
		MDINBUS_SetPage(MDIN_SDRAM_ID, 0x0303);	// write sdram page
		MDINBUS_Read(MDIN_SDRAM_ID, idx, pBuff, len);
		bytes-=len; rAddr+=len; pBuff+=len;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
static void BUS_WriteByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	switch (nID&0xfe) {
		case MDIN_HOST_ID:	MHOST_BUSWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	LOCAL_BUSWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_HDMI_ID:	MHDMI_BUSWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	SDRAM_BUSWrite(rAddr, (PBYTE)pBuff, bytes); break;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
static void BUS_ReadByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	switch (nID&0xfe) {
		case MDIN_HOST_ID:	MHOST_BUSRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	LOCAL_BUSRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_HDMI_ID:	MHDMI_BUSRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	SDRAM_BUSRead(rAddr, (PBYTE)pBuff, bytes); break;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_SetPageID(WORD nID)
{
	PageID = nID;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	BUS_WriteByID(nID, rAddr, (PBYTE)pBuff, bytes);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_RegWrite(BYTE nID, DWORD rAddr, WORD wData)
{
	MDINBUS_MultiWrite(nID, rAddr, (PBYTE)&wData, 2);	// only 2-byte
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	BUS_ReadByID(nID, rAddr, (PBYTE)pBuff, bytes);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_RegRead(BYTE nID, DWORD rAddr, PWORD rData)
{
	MDINBUS_MultiRead(nID, rAddr, (PBYTE)rData, 2);		// only 2-byte
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData)
{
	WORD temp;

	if (bPos>15||bCnt==0||bCnt>16||(bPos+bCnt)>16) return MDIN_INVALID_PARAM;
	MDINBUS_RegRead(nID, rAddr, &temp);
	bCnt = ~(0xffff<<bCnt);
	temp &= ~(bCnt<<bPos);
	temp |= ((bData&bCnt)<<bPos);
	MDINBUS_RegWrite(nID, rAddr, temp);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_SetDMA(DWORD rAddr, WORD bytes, BOOL dir)
{
#if   CPU_ACCESS_BUS_NBYTE == 1 	// bus width = 16bit
	WORD row, col, num, ctl = (bytes>256)? 0x0a : 0x02;
#else 								// bus width = 8bit
	WORD row, col, num, ctl = (bytes>512)? 0x0a : 0x02;
#endif

	row = ADDR2ROW(rAddr, 4096);	// get row
	col = ADDR2COL(rAddr, 4096);	// get col

	if (dir==DMA_RD) ctl = 0x03; num = bytes-1;
	MDINBUS_RegWrite(MDIN_HOST_ID, 0x006, row);		// set DMA row+bank address
	MDINBUS_RegWrite(MDIN_HOST_ID, 0x008, col);		// set DMA column address
	MDINBUS_RegWrite(MDIN_HOST_ID, 0x00a, num);		// set DMA counter (read)
	MDINBUS_RegWrite(MDIN_HOST_ID, 0x014, num);		// set DMA block counter
	MDINBUS_RegWrite(MDIN_HOST_ID, 0x00c, ctl);		// set DMA block start
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for BUS read & BUS write
// You must make functions which is defined below.
//--------------------------------------------------------------------------------------------------------------------------
static void CS_ALT(WORD delay)
{
	while (--delay);	// CPU dependent, CS must do toggle at 1-data units.
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINBUS_Write(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD i;

#if   CPU_ACCESS_BUS_NBYTE == 1 	// bus width = 16bit, ignore endian
		for (i=0; i<bytes/2; i++) {
			*(MDIN380_BASE_ADDR+(rAddr+i*2+0)) = LOWORD(((PWORD)pBuff)[i]);
			CS_ALT(10);	// CPU dependent, CS must do toggle at 1-word units.
		}
#else								// bus width = 8bit, ignore endian
	if (nID==MDIN_SDRAM_ID) {
		for (i=0; i<bytes/1; i++) {
			*(MDIN380_BASE_ADDR+(rAddr+i*1+0)) = LOBYTE(((PBYTE)pBuff)[i]);
			CS_ALT(10);	// CPU dependent, CS must do toggle at 1-byte units.
		}
	}
	else {
		for (i=0; i<bytes/2; i++) {
			*(MDIN380_BASE_ADDR+(rAddr+i*2+1)) = HIBYTE(((PWORD)pBuff)[i]);
			CS_ALT(10);	// CPU dependent, CS must do toggle at 1-byte units.
			*(MDIN380_BASE_ADDR+(rAddr+i*2+0)) = LOBYTE(((PWORD)pBuff)[i]);
			CS_ALT(10);	// CPU dependent, CS must do toggle at 1-byte units.
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINBUS_Read(BYTE nID, WORD rAddr, PBYTE pBuff, WORD bytes)
{
	WORD i;

#if   CPU_ACCESS_BUS_NBYTE == 1 	// bus width = 16bit, ignore endian
		for (i=0; i<bytes/2; i++) {
			((PWORD)pBuff)[i]  = *(MDIN380_BASE_ADDR+(rAddr+i*2+0))<<0;
		}
#else								// bus width = 8bit, ignore endian
	if (nID==MDIN_SDRAM_ID) {
		for (i=0; i<bytes/1; i++) {
			((PBYTE)pBuff)[i]  = *(MDIN380_BASE_ADDR+(rAddr+i*1+0))<<0;
		}
	}
	else {
		for (i=0; i<bytes/2; i++) {
			((PWORD)pBuff)[i]  = *(MDIN380_BASE_ADDR+(rAddr+i*2+1))<<8;
			CS_ALT(2);	// CPU dependent, CS must do toggle at 1-byte units.
			((PWORD)pBuff)[i] |= *(MDIN380_BASE_ADDR+(rAddr+i*2+0))<<0;
		}
	}
 #endif
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_DMAWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD unit = 4096/2;		// write at 1-bank unit.
	if (nID!=MDIN_SDRAM_ID) return MDIN_INVALID_PARAM;

	while (bytes>0) {
		WORD i, len = MIN(unit-(rAddr%unit), bytes);

//--------------------------------------------------------
// TODO: Add your control notification handler code here |

#if   CPU_ACCESS_BUS_NBYTE == 1 		// bus width = 16bit, ignore endian
		MDINBUS_SetDMA(rAddr, len/2, DMA_WR);	// set MDIN DMA operation
		while (GET_MDIN_DREQ()==HIGH);			// wait for /DREQ

		EPIDividerSet(EPI0_BASE, 1);	// CPU dependent, fast access mode
		for (i=0; i<len/2; i++) {
			*(MDIN380_BASE_ADDR) = ((PWORD)pBuff)[i];
		}
		EPIDividerSet(EPI0_BASE, 4);	// CPU dependent, normal access mode
#else									// bus width = 8bit, ignore endian
		MDINBUS_SetDMA(rAddr, len/1, DMA_WR);	// set MDIN DMA operation
		while (GET_MDIN_DREQ()==HIGH);			// wait for /DREQ

		EPIDividerSet(EPI0_BASE, 1);	// CPU dependent, fast access mode
		for (i=0; i<len/1; i++) {
			*(MDIN380_BASE_ADDR) = ((PBYTE)pBuff)[i];
		}
		EPIDividerSet(EPI0_BASE, 4);	// CPU dependent, normal access mode
 #endif

// TODO: End your control notification handler code here |
//--------------------------------------------------------

		rAddr += len; pBuff += len; bytes -= len;
	}

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINBUS_DMARead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	if (nID!=MDIN_SDRAM_ID) return MDIN_INVALID_PARAM;

	while (bytes>0) {
		WORD i, len = MIN(512, bytes);	// read at 512-byte unit.

//--------------------------------------------------------
// TODO: Add your control notification handler code here |

		MDINBUS_SetDMA(rAddr, len/1, DMA_RD);	// set MDIN DMA operation
		while (GET_MDIN_DREQ()==HIGH);			// wait for /DREQ

		EPIDividerSet(EPI0_BASE, 1);	// CPU dependent, fast access mode
		for (i=0; i<len/1; i++) {
			((PBYTE)pBuff)[i] = *(MDIN380_BASE_ADDR);
		}
		EPIDividerSet(EPI0_BASE, 4);	// CPU dependent, normal access mode

// TODO: End your control notification handler code here |
//--------------------------------------------------------

		rAddr += len; pBuff += len; bytes -= len;
	}
	return MDIN_NO_ERROR;
}

#endif	/* SYSTEM_USE_BUS_HIF */

/*  FILE_END _HERE */
