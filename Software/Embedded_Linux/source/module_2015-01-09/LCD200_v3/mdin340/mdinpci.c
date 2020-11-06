//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINPCI.C
// Description 		:  Host interface Protocol handling
// Ref. Docment		: 
// Revision History 	:

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include	<string.h>
#include	"mdin3xx.h"

#if defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_PCI_HIF)
#include	<sys/ioctl.h>     // ioctl

// ----------------------------------------------------------------------
// Struct/Union Types and define
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------
extern int fb_fd;

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------
static void MDINPCI_Write(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
static void MDINPCI_Read(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);

// ----------------------------------------------------------------------
// Global functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
static void MHOST_PCIWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_MHOST_OFFSET+rAddr*4;
	MDINPCI_Write(MDIN_HOST_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHOST_PCIRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_MHOST_OFFSET+rAddr*4;
	MDINPCI_Read(MDIN_HOST_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void LOCAL_PCIWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_LOCAL_OFFSET+rAddr*4;
	MDINPCI_Write(MDIN_LOCAL_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void LOCAL_PCIRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
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

	LOCAL_PCIWrite(0x3ff, (PBYTE)&RegOEN, 2);	// write reg_oen

	rAddr = MDINPCI_LOCAL_OFFSET+rAddr*4;
	MDINPCI_Read(MDIN_LOCAL_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MHDMI_GetWriteDone(void)
{
	WORD rVal = 0, count = 100;
	
	while (count&&(rVal==0)) {
		MHOST_PCIRead(0x027, (PBYTE)&rVal, 2);
		rVal &= 0x04;	count--;
	}
	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHDMI_PCIWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_MHDMI_OFFSET+rAddr*2;
	MDINPCI_Write(MDIN_HDMI_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void MHDMI_PCIRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	WORD i, waddr, wdata, wctrl, err = 0;

	for (i=0; i<bytes/2; i++) {
		waddr = rAddr+i*2;	MHOST_PCIWrite(0x025, (PBYTE)&waddr, 2);
		wctrl = 0x0003;		MHOST_PCIWrite(0x027, (PBYTE)&wctrl, 2);
		wctrl = 0x0002;		MHOST_PCIWrite(0x027, (PBYTE)&wctrl, 2);

		err = MHDMI_GetWriteDone(); if (err) break;	// check done flag
		MHOST_PCIRead(0x026, (PBYTE)&wdata, 2);	((PWORD)pBuff)[i] = wdata;
	}

	if (err==MDIN_TIMEOUT_ERROR) mdinERR = 4;
}

//--------------------------------------------------------------------------------------------------------------------------
static void SDRAM_PCIWrite(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_SDRAM_OFFSET+rAddr*1;
	MDINPCI_Write(MDIN_SDRAM_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void SDRAM_PCIRead(DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	rAddr = MDINPCI_SDRAM_OFFSET+rAddr*1;
	MDINPCI_Read(MDIN_SDRAM_ID, rAddr, pBuff, bytes);
}

//--------------------------------------------------------------------------------------------------------------------------
static void PCI_WriteByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	switch (nID&0xfe) {
		case MDIN_HOST_ID:	MHOST_PCIWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	LOCAL_PCIWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_HDMI_ID:	MHDMI_PCIWrite(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	SDRAM_PCIWrite(rAddr, (PBYTE)pBuff, bytes); break;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
static void PCI_ReadByID(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	switch (nID&0xfe) {
		case MDIN_HOST_ID:	MHOST_PCIRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_LOCAL_ID:	LOCAL_PCIRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_HDMI_ID:	MHDMI_PCIRead(rAddr, (PBYTE)pBuff, bytes); break;
		case MDIN_SDRAM_ID:	SDRAM_PCIRead(rAddr, (PBYTE)pBuff, bytes); break;
	}
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	PCI_WriteByID(nID, rAddr, (PBYTE)pBuff, bytes);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_RegWrite(BYTE nID, DWORD rAddr, WORD wData)
{
	MDINPCI_MultiWrite(nID, rAddr, (PBYTE)&wData, 2);	// only 2-byte
	return MDIN_NO_ERROR;	
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{ 
	PCI_ReadByID(nID, rAddr, (PBYTE)pBuff, bytes);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_RegRead(BYTE nID, DWORD rAddr, PWORD rData)
{
	MDINPCI_MultiRead(nID, rAddr, (PBYTE)rData, 2);		// only 2-byte
	return MDIN_NO_ERROR;	
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData)
{
	WORD temp;

	if (bPos>15||bCnt==0||bCnt>16||(bPos+bCnt)>16) return MDIN_INVALID_PARAM;
	MDINPCI_RegRead(nID, rAddr, &temp);
	bCnt = ~(0xffff<<bCnt);
	temp &= ~(bCnt<<bPos);
	temp |= ((bData&bCnt)<<bPos);
	MDINPCI_RegWrite(nID, rAddr, temp);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_MMAPWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	if (nID!=MDIN_SDRAM_ID) return MDIN_INVALID_PARAM;

	if (MDINHIF_RegField(MDIN_HOST_ID,  0x005,  11,  2,  3)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_HIGH)) return MDIN_I2C_ERROR;
	
	memcpy((PBYTE)rAddr, pBuff, bytes);

	if (MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_NORM)) return MDIN_I2C_ERROR;

#if	CPU_ALIGN_BIG_ENDIAN == 1
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x005,  11,  2,  1)) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINPCI_MMAPRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	if (nID!=MDIN_SDRAM_ID) return MDIN_INVALID_PARAM;

	if (MDINHIF_RegField(MDIN_HOST_ID,  0x006,  14,  2,  3)) return MDIN_I2C_ERROR;
	if (MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_HIGH)) return MDIN_I2C_ERROR;
	
	memcpy(pBuff, (PBYTE)rAddr, bytes);

	if (MDIN3xx_SetPriorityHIF(MDIN_PRIORITY_NORM)) return MDIN_I2C_ERROR;

#if	CPU_ALIGN_BIG_ENDIAN == 1
	if (MDINHIF_RegField(MDIN_HOST_ID,  0x006,  14,  2,  1)) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for PCI read & PCI write
//--------------------------------------------------------------------------------------------------------------------------
static void MDINPCI_Write(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	MDINPCI_STRUCT stREG;	DWORD i, wBuff;

	bytes = (nID==MDIN_SDRAM_ID)? bytes/4 : bytes/2;

	for (i=0; i<bytes; i++) {
		if (nID==MDIN_SDRAM_ID) wBuff = ((PDWORD)pBuff)[i];
		else					wBuff = ((PWORD) pBuff)[i];

		stREG.addr = rAddr+i*4; stREG.pBuff = &wBuff;
		ioctl(fb_fd, MDINPCI_REG_WRITE, &stREG);
	}
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINPCI_Read(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes)
{
	MDINPCI_STRUCT stREG;	DWORD i, wBuff;

	bytes = (nID==MDIN_SDRAM_ID)? bytes/4 : bytes/2;

	for (i=0; i<bytes; i++) {
		stREG.addr = rAddr+i*4; stREG.pBuff = &wBuff;
		ioctl(fb_fd, MDINPCI_REG_READ, &stREG);

		if (nID==MDIN_SDRAM_ID) ((PDWORD)pBuff)[i] = wBuff;
		else					((PWORD) pBuff)[i] = wBuff;
	}
}

#endif	/* SYSTEM_USE_PCI_HIF */

/*  FILE_END _HERE */
