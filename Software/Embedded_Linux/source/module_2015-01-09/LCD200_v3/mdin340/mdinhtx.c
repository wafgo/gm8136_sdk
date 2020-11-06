//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2008  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:	HDMITX.C
// Description 		:
// Ref. Docment		: 
// Revision History 	:

// ----------------------------------------------------------------------
// Include files
// ----------------------------------------------------------------------
#include	<linux/string.h>
#include	"mdin3xx.h"

#if __MDINHTX_DBGPRT__ == 1
#include	"..\common\ticortex.h"	// for UARTprintf
#endif

#if	defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380)

// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Global Data section variables
// ----------------------------------------------------------------------
static BOOL SetPAGE, GetHDMI = 0;
static BYTE GetEDID, GetPLUG, GetMDDC;

#if __MDINHTX_DBGPRT__ == 1
static BYTE OldPROC = 0xff;
#endif

#if SYSTEM_USE_HTX_HDCP == 1
static BYTE GetRPT, GetSAME, GetHDCP;
static WORD GetCMD, GetMODE, GetFIFO;

#if __MDINHTX_DBGPRT__ == 1
static WORD GetFAIL = 0;
static BYTE OldAUTH = 0xff;
#endif
#endif

// ----------------------------------------------------------------------
// External Variable 
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Static Prototype Functions
// ----------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_InitModeDVI(PMDIN_VIDEO_INFO pINFO);

// ----------------------------------------------------------------------
// Static functions
// ----------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for MDDC interface
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetMDDCProcDone(void)
{
	WORD rVal = 0x10, count = 100;

	while (count&&(rVal==0x10)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0f2, &rVal)) return MDIN_I2C_ERROR;
		rVal &= 0x10;	count--;	MDINDLY_10uSec(10);		// delay 100us
	}

#if __MDINHTX_DBGPRT__ == 1
	if (count==0) UARTprintf("DDC method is failure. DDC FIFO is busy.\n");
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetMDDCCmd(PMDIN_HDMIMDDC_INFO pMDDC)
{
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x0ec, (PBYTE)pMDDC, 6)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0900)) return MDIN_I2C_ERROR;	// clear FIFO

	if (pMDDC->cmd==0x06) return MDIN_NO_ERROR;		// sequential write commnad
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, MAKEWORD(pMDDC->cmd,0))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetMDDCStatus(void)
{
	WORD rVal;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0f2, &rVal)) return MDIN_I2C_ERROR;

	// check BUS_LOW, NO_ACK, IN_PROG, FIFO_FULL
	if ((rVal&0x78)==0) return MDIN_NO_ERROR;

	// can happen if Rx is clock stretching the SCL line. DDC bus unusable
	if ((rVal&0x20)!=0) return MDIN_DDC_ACK_ERROR;

	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0f00)) return MDIN_I2C_ERROR;	// ABORT
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0a00)) return MDIN_I2C_ERROR;	// CLOCK
	return MDIN_NO_ERROR;
}

#if SYSTEM_USE_HTX_HDCP == 1
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetRiStatus(void)
{
	WORD rVal = 0x01, count = 1000;

	while (count&&(rVal==0x01)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x026, &rVal)) return MDIN_I2C_ERROR;
		rVal &= 0x01; count--; MDINDLY_mSec(10);	// delay 10ms
	}

#if __MDINHTX_DBGPRT__ == 1
	if (count==0) UARTprintf("MDDC bus not relinquished.\n");
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SuspendRiCheck(BOOL OnOff)
{
	MDIN_ERROR_t err = MDIN_NO_ERROR;

	if (OnOff==ON) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x026, &GetCMD)) return MDIN_I2C_ERROR;
		GetCMD = HIBYTE(GetCMD)&0x03;	// Save RI_CMD status
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;

		// wait for HW to release MDDC bus
		err = MDINHTX_GetRiStatus(); if (err==MDIN_I2C_ERROR) return err;
		if (err==MDIN_TIMEOUT_ERROR) {	// MDDC bus not relinquished.
			return MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 8, 8, GetCMD);
		}
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x026, &GetMODE)) return MDIN_I2C_ERROR;
		GetMODE = LOBYTE(GetMODE)&0x01;	// Save MDDC bus status
		return MDIN_NO_ERROR;
	}

	if (GetMODE&&(GetCMD&0x01)) {
		if (MDINHTX_EnableRISCheck(ON)) return MDIN_I2C_ERROR;	// re-enable Auto Ri
	}

	if (GetCMD&0x02) {
		if (MDINHTX_EnableKSVReady(ON)) return MDIN_I2C_ERROR;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetMDDCBuff(PMDIN_HDMIMDDC_INFO pMDDC)
{
	WORD i, rVal;

	for (i=0; i<pMDDC->bytes; i++) {
		rVal = (WORD)(pMDDC->pBuff[i]);
		if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f4, rVal)) return MDIN_I2C_ERROR;
	}
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, MAKEWORD(pMDDC->cmd,0))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_WriteMDDC(PMDIN_HDMIMDDC_INFO pMDDC)
{
	MDIN_ERROR_t err;

	if (MDINHTX_SuspendRiCheck(ON)) return MDIN_I2C_ERROR;

	err = MDINHTX_GetMDDCProcDone(); if (err==MDIN_I2C_ERROR) return err;

	// Abort Master DCC operation and Clear FIFO pointer
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0900)) return MDIN_I2C_ERROR;

	if (MDINHTX_SetMDDCCmd(pMDDC)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetMDDCBuff(pMDDC)) return MDIN_I2C_ERROR;
	err = MDINHTX_GetMDDCProcDone(); if (err==MDIN_I2C_ERROR) return err;

	err = MDINHTX_GetMDDCStatus(); if (err==MDIN_I2C_ERROR) return err;
	if (err) GetMDDC = MDIN_DDC_ACK_ERROR;

#if __MDINHTX_DBGPRT__ == 1
	if (err) UARTprintf("DDC Write method is failure. No ACK.\n");
#endif

	if (MDINHTX_SuspendRiCheck(OFF)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
#endif	/* SYSTEM_USE_HTX_HDCP == 1 */

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetMDDCBuff(PMDIN_HDMIMDDC_INFO pMDDC)
{
	WORD i, rVal;

	for (i=0; i<pMDDC->bytes; i++) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0f4, &rVal)) return MDIN_I2C_ERROR;
		pMDDC->pBuff[i] = LOBYTE(rVal);
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ReadMDDC(PMDIN_HDMIMDDC_INFO pMDDC)
{
	MDIN_ERROR_t err;

#if SYSTEM_USE_HTX_HDCP == 1
	if (MDINHTX_SuspendRiCheck(ON)) return MDIN_I2C_ERROR;
#endif

	err = MDINHTX_GetMDDCProcDone(); if (err==MDIN_I2C_ERROR) return err;

	// Abort Master DCC operation and Clear FIFO pointer
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0900)) return MDIN_I2C_ERROR;

	if (MDINHTX_SetMDDCCmd(pMDDC)) return MDIN_I2C_ERROR;
	err = MDINHTX_GetMDDCProcDone(); if (err==MDIN_I2C_ERROR) return err;

	if (MDINHTX_GetMDDCBuff(pMDDC)) return MDIN_I2C_ERROR;

	err = MDINHTX_GetMDDCStatus(); if (err==MDIN_I2C_ERROR) return err;
	if (err) GetMDDC = MDIN_DDC_ACK_ERROR;

#if __MDINHTX_DBGPRT__ == 1
	if (err) UARTprintf("DDC Read method is failure. No ACK.\n");
#endif

#if SYSTEM_USE_HTX_HDCP == 1
	if (MDINHTX_SuspendRiCheck(OFF)) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for EDID Parsing
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ReadEDID(BYTE rAddr, PBYTE pBuff, WORD bytes)
{
	MDIN_HDMIMDDC_INFO stMDDC; WORD rVal;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0ee, &rVal)) return MDIN_I2C_ERROR;
	rVal = LOBYTE(rVal);

	stMDDC.sAddr	= MAKEWORD(0xa0, 0);
	stMDDC.rAddr	= MAKEWORD(rAddr, rVal);
	stMDDC.bytes	= bytes;
	stMDDC.pBuff	= pBuff;
	stMDDC.cmd		= (rVal)? 4 : 2;	// enhanced read or sequential read
	return MDINHTX_ReadMDDC(&stMDDC);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseHeader(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE i, rBuff[8];

	if (MDINHTX_ReadEDID(0x00, rBuff, 8)) return MDIN_I2C_ERROR;

	if ((rBuff[0]|rBuff[7])) pCTL->err = EDID_BAD_HEADER;

	for (i=1; i<7; i++) {
		if (rBuff[i]!=0xff) pCTL->err = EDID_BAD_HEADER;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseVersion(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE rBuff[2];

	if (MDINHTX_ReadEDID(0x12, rBuff, 2)) return MDIN_I2C_ERROR;

	if ((rBuff[0]!=1)||(rBuff[1]<3)) pCTL->err = EDID_VER_NOT861B;
	else							 pCTL->err = MDIN_NO_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_CheckCRC(BYTE rAddr)
{
	BYTE i, j, rBuff[8], CRC = 0;

	for (i=0; i<16; i++) {
		if (MDINHTX_ReadEDID(rAddr+i*8, rBuff, 8)) return MDIN_I2C_ERROR;

		for (j=0; j<8; j++) CRC += rBuff[j];
	}
	return (CRC)? MDIN_DDC_CRC_ERROR : MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_Check1stCRC(PMDIN_HDMICTRL_INFO pCTL)
{
	MDIN_ERROR_t err;

	err = MDINHTX_CheckCRC(0x00);
	if (err!=MDIN_DDC_CRC_ERROR) return err;

	pCTL->err = EDID_1ST_CRC_ERR;
	if (MDINHTX_ParseVersion(pCTL)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_Check2ndCRC(PMDIN_HDMICTRL_INFO pCTL)
{
	MDIN_ERROR_t err;

	err = MDINHTX_CheckCRC(0x80);
	if (err!=MDIN_DDC_CRC_ERROR) return err;

	pCTL->err = EDID_2ND_CRC_ERR;
	if (MDINHTX_ParseVersion(pCTL)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_BasicParse(PMDIN_HDMICTRL_INFO pCTL)
{
	if (MDINHTX_ParseHeader(pCTL)) return MDIN_I2C_ERROR;
	if (pCTL->err) return MDIN_NO_ERROR;

	if (MDINHTX_Check1stCRC(pCTL)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetVendorAddr(PBYTE pBuff)
{
	BYTE rAddr, eAddr, rBuff[2];

	rAddr = (SetPAGE)? 0x02 : 0x82;
	if (MDINHTX_ReadEDID(rAddr, rBuff, 2)) return MDIN_I2C_ERROR;
	if (rBuff[0]<4) return MDIN_NO_ERROR;

	rAddr = ((SetPAGE)? 0x04 : 0x84);
	eAddr = ((SetPAGE)? 0x03 : 0x80) + rBuff[0];

	while (rAddr<eAddr) {
		if (MDINHTX_ReadEDID(rAddr, rBuff, 1)) return MDIN_I2C_ERROR;

		if ((rBuff[0]&0xe0)==0x60) {*pBuff = rAddr; break;}
		rAddr += ((rBuff[0]&0x1f) + 1);
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_CheckHDMISignature(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE rAddr = 0, rBuff[3];

	pCTL->phy = 0xffff;	// clear physical address
	if (MDINHTX_GetVendorAddr(&rAddr)) return MDIN_I2C_ERROR;

	// HDMI Signature block not found
	if (rAddr==0) {pCTL->err = EDID_VER_NOTHDMI; return MDIN_NO_ERROR;}

	if (MDINHTX_ReadEDID(rAddr+1, rBuff, 3)) return MDIN_I2C_ERROR;

	if (rBuff[0]!=0x03||rBuff[1]!=0x0c||rBuff[2]!=0x00) {
		pCTL->err = EDID_VER_NOTHDMI; return MDIN_NO_ERROR;
	}

	if (MDINHTX_ReadEDID(rAddr+4, rBuff, 2)) return MDIN_I2C_ERROR;
	pCTL->phy = MAKEWORD(rBuff[0], rBuff[1]);

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("PhyAddr = 0x%04x\n", pCTL->phy);
#endif

	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowParseDetailedTDPart1(PBYTE pBuff)
{
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf(" H Active %d ", MAKEWORD(HI4BIT(pBuff[2]),pBuff[0]));
	UARTprintf(" H Blank %d ", MAKEWORD(LO4BIT(pBuff[2]),pBuff[1]));
	UARTprintf(" V Active %d ", MAKEWORD(HI4BIT(pBuff[5]),pBuff[3]));
	UARTprintf(" V Blank %d ", MAKEWORD(LO4BIT(pBuff[5]),pBuff[4]));
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void  MDINHTX_ShowParseDetailedTDPart2(PBYTE pBuff)
{
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf(" H Sync Offset %d", MAKEWORD(HI4BIT(pBuff[3])>>2,pBuff[0]));
	UARTprintf(" H Sync Pulse W %d", MAKEWORD(HI4BIT(pBuff[3])&3,pBuff[1]));
	UARTprintf(" V Sync Offset %d", MAKEBYTE(LO4BIT(pBuff[3])>>2,HI4BIT(pBuff[2])));
	UARTprintf(" V Sync Pulse W %d", MAKEBYTE(LO4BIT(pBuff[3])&3,LO4BIT(pBuff[2])));
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void  MDINHTX_ShowParseDetailedTDPart3(PBYTE pBuff)
{
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf(" H Image Size %d", MAKEWORD(HI4BIT(pBuff[2]),pBuff[0]));
	UARTprintf(" V Image Size %d", MAKEWORD(LO4BIT(pBuff[2]),pBuff[1]));
	UARTprintf(" H Border %d V Border %d\n", pBuff[3], pBuff[4]);

	if (pBuff[5]&0x80) UARTprintf(" Interlaced");
	else			   UARTprintf(" Non-interlaced");

	if (pBuff[5]&0x60) UARTprintf(" Table 3.17 for defenition:");
	else			   UARTprintf(" Normal Display:");

	if (pBuff[5]&0x10) UARTprintf(" Digital\n");
	else			   UARTprintf(" Analog\n");
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowParseShortTDPart(PBYTE pBuff, BYTE size)
{
#if __MDINHTX_DBGPRT__ == 1
	BYTE i;

	UARTprintf(" CEA861 Modes:\n");
	for (i=0; i<size; i++) {
		UARTprintf(" ID %d",(pBuff[i]&0x7f));
		if (pBuff[i]&0x80) UARTprintf(" Native mode\n");
		else			   UARTprintf(" Non-Native\n");
	}
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseVideoDATABlock(BYTE rAddr)
{
	BYTE bytes, len, rBuff[8];

	if (MDINHTX_ReadEDID(rAddr, &bytes, 1)) return MDIN_I2C_ERROR;

	rAddr++; bytes &= 0x1f;

	while (bytes>0) {
		len = MIN(bytes,8);
		if (MDINHTX_ReadEDID(rAddr, rBuff, len)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseShortTDPart(rBuff, len);	// debug print
		rAddr += len; bytes -= len;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowParseShortADPart(PBYTE pBuff)
{
#if __MDINHTX_DBGPRT__ == 1
	BYTE frmt = (pBuff[0]&0xf8)>>3;
	UARTprintf("Audio Format Code %d ", frmt);

	switch (frmt) {
		case  1: UARTprintf("Liniar PCM\n"); break;
		case  2: UARTprintf("AC-3\n"); break;
		case  3: UARTprintf("MPEG-1\n"); break;
		case  4: UARTprintf("MP3\n"); break;
		case  5: UARTprintf("MPEG2\n"); break;
		case  6: UARTprintf("ACC\n"); break;
		case  7: UARTprintf("DTS\n"); break;
		case  8: UARTprintf("ATRAC\n"); break;
		default: UARTprintf("reserved\n"); break;
	}

	UARTprintf("Max N of channels %d\n", ((pBuff[0]&0x03)+1));
	if (pBuff[1]&0x01) UARTprintf(" Fs: 32KHz\n");
	if (pBuff[1]&0x02) UARTprintf(" Fs: 44KHz\n");
	if (pBuff[1]&0x04) UARTprintf(" Fs: 48KHz\n");
	if (pBuff[1]&0x08) UARTprintf(" Fs: 88KHz\n");
	if (pBuff[1]&0x10) UARTprintf(" Fs: 96KHz\n");
	if (pBuff[1]&0x20) UARTprintf(" Fs: 176KHz\n");
	if (pBuff[1]&0x40) UARTprintf(" Fs: 192KHz\n");

	if (frmt==1) {
		UARTprintf("Supported length: ");
		if (pBuff[2]&0x01) UARTprintf(" 16bits\n");
		if (pBuff[2]&0x02) UARTprintf(" 20bits\n");
		if (pBuff[2]&0x04) UARTprintf(" 24bits\n");
	}
	else UARTprintf(" Maximum bit rate %d KHz\n", (pBuff[2]<<3));
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseAudioDATABlock(BYTE rAddr)
{
	BYTE bytes, len, rBuff[3];

	if (MDINHTX_ReadEDID(rAddr, &bytes, 1)) return MDIN_I2C_ERROR;

	rAddr++; len = 0; bytes &= 0x1f;
	do {
		if (MDINHTX_ReadEDID(rAddr, rBuff, 3)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseShortADPart(rBuff);	// debug print
		len += 3;	rAddr += 3;
	}	while (len<bytes);

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseSpeakDATABlock(BYTE rAddr)
{
#if __MDINHTX_DBGPRT__ == 1
	BYTE rVal;

	if (MDINHTX_ReadEDID(rAddr+1, &rVal, 1)) return MDIN_I2C_ERROR;

	if (rVal&0x01) UARTprintf("Speakers' allocation: FL/FR\n");
	if (rVal&0x02) UARTprintf("Speakers' allocation: LFE\n");
	if (rVal&0x04) UARTprintf("Speakers' allocation: FC\n");
	if (rVal&0x08) UARTprintf("Speakers' allocation: RL/RR\n");
	if (rVal&0x10) UARTprintf("Speakers' allocation: RC\n");
	if (rVal&0x20) UARTprintf("Speakers' allocation: FLC/FRC\n");
	if (rVal&0x40) UARTprintf("Speakers' allocation: RLC/RRC\n");
#endif
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseVSpecDATABlock(BYTE rAddr)
{
	BYTE rBuff[3], phyAddr[2];

	if (MDINHTX_ReadEDID(rAddr+1, rBuff, 3)) return MDIN_I2C_ERROR;

	if (rBuff[0]!=0x03||rBuff[1]!=0x0c||rBuff[2]!=0x00) return MDIN_I2C_ERROR;

	if (MDINHTX_ReadEDID(rAddr+4, phyAddr, 2)) return MDIN_I2C_ERROR;

//	CECSetPhysicalAddress(phyAddr);

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("PhyAddr : %d.%d.%d.%d\n", HI4BIT(phyAddr[0]), LO4BIT(phyAddr[0]), HI4BIT(phyAddr[1]), LO4BIT(phyAddr[1]));
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetParseCEADATABlock(void)
{
	BYTE rAddr, tAddr, rVal, err = 0;

	rAddr = (SetPAGE)? 0x02 : 0x82;
	if (MDINHTX_ReadEDID(rAddr, &rVal, 1)) return MDIN_I2C_ERROR;
	
	rAddr = (SetPAGE)? 0x04 : 0x84; rVal += rAddr;
	do {
		if (MDINHTX_ReadEDID(rAddr, &tAddr, 1)) return MDIN_I2C_ERROR;
		switch (tAddr&0xe0) {
			case 0x40: err = MDINHTX_ParseVideoDATABlock(rAddr); break;
			case 0x20: err = MDINHTX_ParseAudioDATABlock(rAddr); break;
			case 0x80: err = MDINHTX_ParseSpeakDATABlock(rAddr); break;
			case 0x60: err = MDINHTX_ParseVSpecDATABlock(rAddr); break;
		}
		if (err) break;	rAddr += (tAddr&0x1f)+1;   // next Tag Address
	}	while (rAddr<rVal);

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetParseEDIDDTDBlock(void)
{
	BYTE rAddr, eAddr, rBuff[6];

	rAddr = (SetPAGE)? 0x02 : 0x82;
	if (MDINHTX_ReadEDID(rAddr, rBuff, 1)) return MDIN_I2C_ERROR;

	rAddr = rBuff[0] + (SetPAGE)? 0x00 : 0x80;
	if (MDINHTX_ReadEDID(rAddr, rBuff, 2)) return MDIN_I2C_ERROR;

	eAddr = (SetPAGE)? 0x6c : 0xec;
	while (rBuff[0]||rBuff[1]) {
		if (MDINHTX_ReadEDID(rAddr+2, rBuff, 6)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart1(rBuff);	// debug print

		if (MDINHTX_ReadEDID(rAddr+8, rBuff, 4)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart2(rBuff);	// debug print

		if (MDINHTX_ReadEDID(rAddr+12, rBuff, 6)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart3(rBuff);	// debug print

		if (rAddr>eAddr) break;		rAddr += 18;

		if (MDINHTX_ReadEDID(rAddr, rBuff, 2)) return MDIN_I2C_ERROR;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetParse861BExtension(void)
{
	if (MDINHTX_GetParseCEADATABlock()) return MDIN_I2C_ERROR;
	if (MDINHTX_GetParseEDIDDTDBlock()) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowParseEstablishTiming(PBYTE pBuff)
{
#if __MDINHTX_DBGPRT__ == 1
	if (pBuff[0]&0x80) UARTprintf(" 720 x  400 @ 70Hz\n");
	if (pBuff[0]&0x40) UARTprintf(" 720 x  400 @ 88Hz\n");
	if (pBuff[0]&0x20) UARTprintf(" 640 x  480 @ 60Hz\n");
	if (pBuff[0]&0x10) UARTprintf(" 640 x  480 @ 67Hz\n");
	if (pBuff[0]&0x08) UARTprintf(" 640 x  480 @ 72Hz\n");
	if (pBuff[0]&0x04) UARTprintf(" 640 x  480 @ 75Hz\n");
	if (pBuff[0]&0x02) UARTprintf(" 800 x  600 @ 56Hz\n");
	if (pBuff[0]&0x01) UARTprintf(" 800 x  400 @ 60Hz\n");

	if (pBuff[1]&0x80) UARTprintf(" 800 x  600 @ 72Hz\n");
	if (pBuff[1]&0x40) UARTprintf(" 800 x  600 @ 75Hz\n");
	if (pBuff[1]&0x20) UARTprintf(" 832 x  624 @ 75Hz\n");
	if (pBuff[1]&0x10) UARTprintf("1024 x  768 @ 87Hz\n");
	if (pBuff[1]&0x08) UARTprintf("1024 x  768 @ 60Hz\n");
	if (pBuff[1]&0x04) UARTprintf("1024 x  768 @ 70Hz\n");
	if (pBuff[1]&0x02) UARTprintf("1024 x  768 @ 75Hz\n");
	if (pBuff[1]&0x01) UARTprintf("1280 x 1024 @ 75Hz\n");

	if (pBuff[2]&0x80) UARTprintf("1152 x  870 @ 75Hz\n");

	if ((pBuff[0]==0)&&(pBuff[1]==0)&&(pBuff[2]==0))
		UARTprintf("No established video modes\n");
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowParseStandardTiming(PBYTE pBuff, BYTE mode)
{
#if __MDINHTX_DBGPRT__ == 1
	BYTE i, TmpVal;

	mode *= 4; TmpVal = mode+4;
	for (i=0; mode<TmpVal; mode++) {
		i++;
		UARTprintf(" Mode %d ", mode);
		if ((pBuff[i]==0x01)&&(pBuff[i+1]==0x01)) {
			UARTprintf(" wasn't defined!\n");
		}
		else {
			UARTprintf(" Hor Act pixels %d ", ((pBuff[mode]+31)*8));
			UARTprintf(" Aspect ratio: ");
			TmpVal = pBuff[mode+1]&0xc0;
			if(TmpVal==0x00)		UARTprintf("16:10");
			else if(TmpVal==0x40)	UARTprintf("4:3");
			else if(TmpVal==0x80)	UARTprintf("5:4");
			else					UARTprintf("16:9");
			UARTprintf(" Refresh rate %d Hz\n", ((pBuff[mode+1])&0x3f)+60);
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetParseTimingDesc(void)
{
	BYTE i, rBuff[8];

	if (MDINHTX_ReadEDID(0x23, rBuff, 3)) return MDIN_I2C_ERROR;
	MDINHTX_ShowParseEstablishTiming(rBuff);	// debug print

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("Standard Timing:\n");
#endif

	if (MDINHTX_ReadEDID(0x26, rBuff, 8)) return MDIN_I2C_ERROR;
	MDINHTX_ShowParseStandardTiming(rBuff, 0);	// debug print

	if (MDINHTX_ReadEDID(0x2e, rBuff, 8)) return MDIN_I2C_ERROR;
	MDINHTX_ShowParseStandardTiming(rBuff, 1);	// debug print

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf(" Detailed Timing: \n");
#endif

	for (i=0; i<4; i++) {

#if __MDINHTX_DBGPRT__ == 1
		UARTprintf(" Descriptor %d:\n", i);
#endif

		if (MDINHTX_ReadEDID(0x36+i*18, rBuff, 4)) return MDIN_I2C_ERROR;
		if (rBuff[0]==0&&rBuff[1]==0) continue;

		if (MDINHTX_ReadEDID(0x38+i*18, rBuff, 6)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart1(rBuff);	// debug print

		if (MDINHTX_ReadEDID(0x3e+i*18, rBuff, 4)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart2(rBuff);	// debug print

		if (MDINHTX_ReadEDID(0x42+i*18, rBuff, 6)) return MDIN_I2C_ERROR;
		MDINHTX_ShowParseDetailedTDPart3(rBuff);	// debug print
	}
	return MDIN_NO_ERROR;
}
*/
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetNativeModeBy861B(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE rVal;

	if (MDINHTX_ReadEDID(0x83, &rVal, 1)) return MDIN_I2C_ERROR;

	switch (rVal&0x30) {
		case 0x10:	pCTL->mode = HDMI_OUT_SEP422_8; break;
		case 0x20:
		case 0x30:	pCTL->mode = HDMI_OUT_YUV444_8; break;
		default:	pCTL->mode = HDMI_OUT_RGB444_8; break;
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf(" DTV monitor supports: \n");
	if (rVal&0x80) UARTprintf(" Underscan");
	if (rVal&0x40) UARTprintf(" Basic audio");
	if (rVal&0x20) UARTprintf(" YCbCr 4:4:4");
	if (rVal&0x10) UARTprintf(" YCbCr 4:2:2");
	UARTprintf("\n");
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetNativeFrmtByDTD(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE i, rBuff[6]; WORD blkH, blkV, totH, totV, posH, posV;

	pCTL->frmt = 0xff;		// clear native format
	blkH = blkV = totH = totV = posH = posV = 0;

	for (i=0; i<4; i++) {
		if (MDINHTX_ReadEDID(i*18+0x36, rBuff, 2)) return MDIN_I2C_ERROR;
		if (rBuff[0]==0&&rBuff[1]==0) continue;

		if (MDINHTX_ReadEDID(i*18+0x38, rBuff, 6)) return MDIN_I2C_ERROR;
		blkH = MAKEWORD(LO4BIT(rBuff[2]),rBuff[1]);
		blkV = MAKEWORD(LO4BIT(rBuff[5]),rBuff[4]);
		totH = MAKEWORD(HI4BIT(rBuff[2]),rBuff[0]) + blkH;
		totV = MAKEWORD(HI4BIT(rBuff[5]),rBuff[3]) + blkV;

		if (MDINHTX_ReadEDID(i*18+0x3e, rBuff, 4)) return MDIN_I2C_ERROR;
		posH = blkH - MAKEWORD(HI4BIT(rBuff[3])>>2,rBuff[0]);
		posV = blkV - MAKEBYTE(LO4BIT(rBuff[3])>>2,HI4BIT(rBuff[2]));

		break;
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("totH=%d, totV=%d, posH=%d, posV=%d\n", totH, totV, posH, posV);
#endif

	for (i=0; i<VIDOUT_FORMAT_END; i++) {
		if (totH!=defMDINHTXVideo[i].stFREQ.pixels) continue;
		if (totV!=defMDINHTXVideo[i].stFREQ.v_line) continue;
		if (posH!=defMDINHTXVideo[i].stWIND.x) continue;
		if (posV!=defMDINHTXVideo[i].stWIND.y) continue;

		pCTL->frmt = defMDINHTXVideo[i].stMODE.id_1; break;
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("Native Format from DTD = %d\n", pCTL->frmt);
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetNativeFrmtByVID(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE rAddr, tagID, rVal, bytes;

	rAddr = (SetPAGE)? 0x02 : 0x82;
	if (MDINHTX_ReadEDID(rAddr, &rVal, 1)) return MDIN_I2C_ERROR;
	if (rVal==4||rVal==0) return MDIN_NO_ERROR;
	
	rAddr = (SetPAGE)? 0x04 : 0x84; 	bytes = rAddr + rVal;

	while (rAddr<bytes) {
		if (MDINHTX_ReadEDID(rAddr, &tagID, 1)) return MDIN_I2C_ERROR;
		if ((tagID&0xe0)==0x40) break;
		rAddr += ((tagID&0x1f)+1);	// next tag address
	}

	rAddr++; bytes = rAddr + (tagID&0x1f);
	while (rAddr<bytes) {
		if (MDINHTX_ReadEDID(rAddr++, &rVal, 1)) return MDIN_I2C_ERROR;
		if (rVal&0x80) {pCTL->frmt = rVal&0x7f; break;}

	#if __MDINHTX_DBGPRT__ == 1
		UARTprintf("non-Native Format from VID = %d\n", rVal);
	#endif
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("Native Format from VID = %d\n", pCTL->frmt);
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ParseEDID(PMDIN_HDMICTRL_INFO pCTL)
{
	BYTE rVal, rNum, rAddr = 0;

	SetPAGE = 0;	// must preserve proc & auth value
	memset((PBYTE)pCTL, 0, sizeof(MDIN_HDMICTRL_INFO)-2);

	if (MDINHTX_BasicParse(pCTL)) return MDIN_I2C_ERROR;
	if (pCTL->err) return MDIN_NO_ERROR;

	// set native format from detailed timing descriptor
	if (MDINHTX_SetNativeFrmtByDTD(pCTL)) return MDIN_I2C_ERROR;

	// Check 861B extension
	if (MDINHTX_ReadEDID(0x7e, &rNum, 1)) return MDIN_I2C_ERROR;
	if (rNum==0) {
		pCTL->err = EDID_EXT_NOT861B; return MDIN_NO_ERROR;
	}

	// Check CRC, sum of extension (128 BYTEs) must be Zero (0)
	if (MDINHTX_Check2ndCRC(pCTL)) return MDIN_I2C_ERROR;
	if (pCTL->err) return MDIN_NO_ERROR;

	// case of NExtensions = 1
	if (rNum==1) {
		if (MDINHTX_CheckHDMISignature(pCTL)) return MDIN_I2C_ERROR;
		if (pCTL->err) return MDIN_NO_ERROR;
		if (MDINHTX_SetNativeModeBy861B(pCTL)) return MDIN_I2C_ERROR;
		if (MDINHTX_SetNativeFrmtByVID(pCTL)) return MDIN_I2C_ERROR;
		pCTL->type = HTX_DISPLAY_HDMI;
	#if __MDINHTX_DBGPRT__ == 1
		UARTprintf("************************ HDMI Mode!\n");
	#endif
		return MDIN_NO_ERROR;
	}

	// case of NExtensions > 1
	if (MDINHTX_ReadEDID(0x80, &rVal, 1)) return MDIN_I2C_ERROR;
	if (rVal!=0xf0) pCTL->err = EDID_MAP_NOBLOCK;
	if (pCTL->err) return MDIN_NO_ERROR;

	while (rNum--) {	rAddr++;
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0xee, 0, 8, rAddr)) return MDIN_I2C_ERROR;

		if (MDINHTX_ReadEDID(0x00, &rVal, 1)) return MDIN_I2C_ERROR;
		if ((rVal&0x02)==0) continue;

		if (MDINHTX_Check1stCRC(pCTL)) return MDIN_I2C_ERROR;
		if (pCTL->err) break;

		SetPAGE = 1;
		if (MDINHTX_CheckHDMISignature(pCTL)) return MDIN_I2C_ERROR;
		if (pCTL->err) break;

		pCTL->type = HTX_DISPLAY_HDMI;
//		if (MDINHTX_SetNativeModeBy861B(pCTL)) return MDIN_I2C_ERROR;
		if (MDINHTX_SetNativeFrmtByVID(pCTL)) return MDIN_I2C_ERROR;

		SetPAGE = 0;
		if (MDINHTX_Check2ndCRC(pCTL)) return MDIN_I2C_ERROR;
		if (pCTL->err) break;

		if (MDINHTX_SetNativeModeBy861B(pCTL)) return MDIN_I2C_ERROR;
		if (MDINHTX_SetNativeFrmtByVID(pCTL)) return MDIN_I2C_ERROR;
		if (pCTL->type==HTX_DISPLAY_HDMI) break;
	}

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0xee, 0, 8, 0x00)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetParseEDID(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HTXVIDEO_INFO pVID = (PMDIN_HTXVIDEO_INFO)&pINFO->stVID_h;
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("========== Parse EDID (1st) ============\n");
#endif

	GetMDDC = MDIN_NO_ERROR;	// clear MDDC error
	if (MDINHTX_ParseEDID(pCTL)) return MDIN_I2C_ERROR;

#if __MDINHTX_DBGPRT__ == 1
	if (pCTL->err) UARTprintf("EDID Error %02x \n", pCTL->err);
#endif

	if (GetMDDC==MDIN_DDC_ACK_ERROR) {
#if SYSTEM_USE_HTX_HDCP == 1
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		pCTL->auth = HDCP_REAUTH_REQ;	// force re-authentication
#endif
	}

	// increase EDID read count
	GetEDID += (pCTL->err==MDIN_NO_ERROR)? -GetEDID : 1;

	// EDID error, or not supported version
	if (pCTL->err>=8) pCTL->proc = HTX_CABLE_DVI_OUT;
	if (pCTL->err!=0) return MDIN_NO_ERROR;

	if (pCTL->type==HTX_DISPLAY_HDMI)	pCTL->proc = HTX_CABLE_HDMI_OUT;
	else								pCTL->proc = HTX_CABLE_DVI_OUT;

	if (pVID->fine&HDMI_USE_FORCE_DVI)	pCTL->proc = HTX_CABLE_DVI_OUT;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for InfoFrame (AVI, AUD, CP)
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetControlPKTOff(void)
{
	WORD rVal = 0x0800, count = 100;

	while (count&&(rVal==0x0800)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;
//		rVal &= 0x0800; count--;	MDINDLY_10uSec(5);	// delay 50us
		rVal &= 0x0800; count--;	MDINDLY_mSec(1);	// delay 1ms
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("(Disable CP) val=0x%02X, count=%d\n", rVal, count);
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableControlPKT(BOOL OnOff)
{
	MDIN_ERROR_t err;	WORD rVal;

	if (GetHDMI==FALSE) return MDIN_NO_ERROR;	// check sink is HDMI

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x1de, &rVal)) return MDIN_I2C_ERROR;
	if ( OnOff&&(HIBYTE(rVal)==0x01)) return MDIN_NO_ERROR;	// already mute set
	if (!OnOff&&(HIBYTE(rVal)==0x10)) return MDIN_NO_ERROR;	// already mute clear

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 10, 2, 0)) return MDIN_I2C_ERROR;

	err = MDINHTX_GetControlPKTOff(); if (err==MDIN_I2C_ERROR) return err;
	if (err==MDIN_TIMEOUT_ERROR) return MDIN_NO_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x1de, 8, 8, (OnOff)? 0x01 : 0x10)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 10, 2, 3)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetInfoFrameOff(BYTE mask)
{
	WORD rVal = mask, count = 100;

	while (count&&(rVal==mask)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;
		rVal &= mask; count--;	MDINDLY_mSec(1);	// delay 1ms
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("(Disable InfoFrame) val=0x%02X, count=%d\n", rVal, count);
	if (count==0) UARTprintf(" Disable InfoFrame Error!!!\n");
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetInfoFrameAVI(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	BYTE i, mode, len, CRC;	WORD rBuff[7];

	if (OnOff==OFF) return MDIN_NO_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 0, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHTX_GetInfoFrameOff(0x02)) return MDIN_I2C_ERROR;

	memset(rBuff, 0, sizeof(rBuff));	// clear infoframe buff

	// colorimetry information
	mode = defMDINHTXVideo[pINFO->stOUT_m.frmt].stMODE.id_1;
	rBuff[0] |= (mode== 4||mode== 5||mode==16||mode==19||mode==20||mode==31||
				 mode==32||mode==33||mode==34)? (2<<14) : (1<<14);
	if (pINFO->stVID_h.mode==HDMI_OUT_RGB444_8) rBuff[0] &= ~(3<<14);

	// picture aspect ratio information
	mode = defMDINHTXVideo[pINFO->stOUT_m.frmt].stMODE.repl;
	rBuff[0] |= (HI4BIT(mode)==2)? (2<<12) : (1<<12);

	// active format aspect ratio information
	rBuff[0] |= (8<<8); 

	// color space information
	if		(pINFO->stVID_h.mode==HDMI_OUT_RGB444_8) rBuff[0] |= (0<<5);
	else if (pINFO->stVID_h.mode==HDMI_OUT_YUV444_8) rBuff[0] |= (2<<5);
	else											 rBuff[0] |= (1<<5);

	// over/under scan information
	mode = defMDINHTXVideo[pINFO->stOUT_m.frmt].stMODE.id_1;
	rBuff[0] |= ((mode>1)&&(mode<60))? 1 : 2;	// VGA start is 60

	// video identification information
	rBuff[1] |= defMDINHTXVideo[pINFO->stOUT_m.frmt].stMODE.id_1<<8;

	len = sizeof(rBuff)-1;	CRC = 0x82 + 0x02 + len;	// because 1-byte dummy
	for (i=0; i<sizeof(rBuff); i++) CRC += ((PBYTE)rBuff)[i];

	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x140, MAKEWORD(0x02,0x82))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x142, MAKEWORD(0x100-CRC,len))) return MDIN_I2C_ERROR;
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x144, (PBYTE)rBuff, len+1)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 0, 2, 3)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableInfoFrmAVI(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	WORD rVal;

	if (MDINHTX_SetInfoFrameAVI(pINFO, OnOff)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 0, 2, (OnOff)? 3 : 0)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetInfoFrameAUD(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	BYTE i, len, CRC;	WORD rBuff[5];

	if (OnOff==OFF) return MDIN_NO_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 4, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHTX_GetInfoFrameOff(0x20)) return MDIN_I2C_ERROR;

	memset(rBuff, 0, sizeof(rBuff));	// clear infoframe buff

	// audio channel count information
	if (pINFO->stAUD_h.fine&AUDIO_MULTI_CHANNEL) rBuff[0] |= 6;

	len = sizeof(rBuff);	CRC = 0x84 + 0x01 + len;
	for (i=0; i<sizeof(rBuff); i++) CRC += ((PBYTE)rBuff)[i];

	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x180, MAKEWORD(0x01,0x84))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x182, MAKEWORD(0x100-CRC,len))) return MDIN_I2C_ERROR;
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x184, (PBYTE)rBuff, len+0)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 4, 2, 3)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableInfoFrmAUD(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	WORD rVal;

	if (MDINHTX_SetInfoFrameAUD(pINFO, OnOff)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13e, 4, 2, (OnOff)? 3 : 0)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
/*
//--------------------------------------------------------------------------------------------------------------------------
void SendGamutVerticesPacket(PBYTE data)
{
	BYTE RegVal;
	WORD writeCnt, sourceCnt;	PWORD sourcePtr;
	BYTE writeData[18], writeAddr;	PBYTE destPtr;
	SHORT colorBitLength, sourceLeftBitCnt, destLeftBitCnt;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("[]SendGamutVerticesPacket is called. \n");
#endif

	// Send Gamut Metadata packets only if in HDMI mode
	if (GetHDMI==FALSE) return MDIN_NO_ERROR;	// check sink is HDMI

	writeData[0] = 0x0A;
	writeData[1] = 0x00|(data[1]&0x0F);
	if (data[2]&0x20) writeData[1] |= 0x80;
	writeData[2] = ((data[1]>>4)&0x0F)|(0x03<<4);
	writeData[3] = (data[2]&0x1F)|0x00;

	if ((data[3]&0x18)==0x00)		colorBitLength = 8;
	else if ((data[3]&0x18)==0x08)	colorBitLength = 10;
	else							colorBitLength = 12;

	writeData[4] = 0;  // Number_Vertices(high)
	writeData[5] = 4;  // Number_Vertices(low)

	destPtr = (PBYTE)&writeData[6];
	sourcePtr = (PWORD)&(data[4]);
	destLeftBitCnt = 8;

	for (sourceCnt=0; sourceCnt<12; sourceCnt++) {
		WORD sourceData = *sourcePtr;
		sourceLeftBitCnt = colorBitLength;
		sourceData &= 0x0FFF;
		while (sourceLeftBitCnt>0) {
			if (sourceLeftBitCnt>=destLeftBitCnt) {
				*destPtr = (BYTE)(sourceData>>(sourceLeftBitCnt-destLeftBitCnt));
				sourceLeftBitCnt -= destLeftBitCnt;
				destLeftBitCnt = 8;
			}
		    else {
				*destPtr += (BYTE)(sourceData<<(destLeftBitCnt-sourceLeftBitCnt));
				destLeftBitCnt -= sourceLeftBitCnt;
				sourceLeftBitCnt = 0;
		    }
			destPtr++;
		}
		sourcePtr++;
	}

	WriteByteHDMITXP0(0xff, 0x01);
	writeAddr = 0x00;
	for (writeCnt=0; writeCnt<sizeof(writeData); writeCnt++) {
		WriteByteHDMITXP0(writeAddr, writeData[writeCnt]);
		writeAddr++;
	}
	WriteByteHDMITXP0(0xff, 0x00);

	RegVal = ReadByteHDMITXP1(0x3f);
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("(Send GamutVerticesPacket)Read INF_CTRL2 %x\n", RegVal);
#endif
	RegVal |= 0x40;
	RegVal |= 0x80;
	WriteByteHDMITXP1(0x3f, RegVal);

#if __MDINHTX_DBGPRT__ == 1
	RegVal = ReadByteHDMITXP1(0x3f);
	UARTprintf("(Send GamutVerticesPacket)Read INF_CTRL2 after set %x\n", RegVal);
#endif
}
}

//--------------------------------------------------------------------------------------------------------------------------
void SendGamutRangePacket(PBYTE data)
{
	BYTE RegVal;
	WORD writeCnt, sourceCnt;	PWORD sourcePtr;
	BYTE writeData[13], writeAddr;	PBYTE destPtr;
	SHORT colorBitLength, sourceLeftBitCnt, destLeftBitCnt;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("[]SendGamutRangePacket is called. \n");
#endif

	// Send Gamut Metadata packets only if in HDMI mode
	if (GetHDMI==FALSE) return;	// check sink is HDMI

	writeData[0] = 0x0A;
	writeData[1] = 0x00|(data[1]&0x0F);
	if (data[2]&0x20) writeData[1] |= 0x80;
	writeData[2] = ((data[1]>>4)&0x0F)|(0x03<<4);
	writeData[3] = (data[2]&0x1F)|0x80;

	if ((data[3]&0x18)==0x00)		colorBitLength = 8;
	else if ((data[3]&0x18)==0x08)	colorBitLength = 10;
	else							colorBitLength = 12;

	destPtr = (PBYTE)&writeData[4];
	sourcePtr = (PWORD)&(data[4]);
	destLeftBitCnt = 8;

	for (sourceCnt=0; sourceCnt<6; sourceCnt++) {
		WORD sourceData = *sourcePtr;
		sourceLeftBitCnt = colorBitLength;
		sourceData &= 0x0FFF;
		while (sourceLeftBitCnt>0) {
			if (sourceLeftBitCnt>=destLeftBitCnt) {
				*destPtr = (BYTE)(sourceData>>(sourceLeftBitCnt-destLeftBitCnt));
				sourceLeftBitCnt -= destLeftBitCnt;
				destLeftBitCnt = 8;
			}
			else {
				*destPtr += (BYTE)(sourceData<<(destLeftBitCnt-sourceLeftBitCnt));
				destLeftBitCnt -= sourceLeftBitCnt;
				sourceLeftBitCnt = 0;
			}
			destPtr++;
		}
		sourcePtr++;
	}

	WriteByteHDMITXP0(0xff, 0x01);
	writeAddr = 0x00;
	for (writeCnt=0; writeCnt<sizeof(writeData); writeCnt++) {
		WriteByteHDMITXP0(writeAddr, writeData[writeCnt]);
		writeAddr++;
	}

	WriteByteHDMITXP0(0xff, 0x00);
	RegVal = ReadByteHDMITXP1(0x3f);
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("(Send GamutRangePacket)Read INF_CTRL2 %x\n", RegVal);
#endif
	RegVal |= 0x40;
	RegVal |= 0x80;
	WriteByteHDMITXP1(0x3f, RegVal);
}

//--------------------------------------------------------------------------------------------------------------------------
void EnableGamutPacket(void)
{
	BYTE RegVal = ReadByteHDMITXP1(0x3f);
	RegVal |= (0x40|0x80);
	WriteByteHDMITXP1(0x3f, RegVal);
}

//--------------------------------------------------------------------------------------------------------------------------
void DisableGamutPacket(void)
{
	BYTE RegVal = ReadByteHDMITXP1(0x3f);
	RegVal &= ~(0x40|0x80);
	WriteByteHDMITXP1(0x3f, RegVal);
}

//--------------------------------------------------------------------------------------------------------------------------
void SetxvYCCSetting(PBYTE data)
{
	WriteByteHDMITXP0(0x50, 0x00);

	if (data[1]&0x01) {
		if (data[1]&0x04) {
			// "Override internal CSC coefficients" is enable.
			SHORT writeCnt;
			BYTE writeAddr = 0x51;
			BYTE writeLowData, writeHighData;
			WORD sourceData;
			PWORD sourcePtr = (PWORD)&data[2];
			// set CSC coefficients
			for (writeCnt=0; writeCnt<9; writeCnt++) {
				sourceData = *sourcePtr;
				WriteWordHDMITXP0(writeAddr, sourceData);
				writeAddr += 2; sourcePtr++;
			}
			// set RGB Input Offset
			sourceData = *sourcePtr; sourcePtr++;
			WriteWordHDMITXP0(writeAddr, sourceData);
			writeAddr+= 2; sourcePtr++;
			// set Y Output Offset
			sourceData = *sourcePtr; sourcePtr++;
			writeLowData = (BYTE)(sourceData&0x7F); // low 7bits
			writeHighData = (BYTE)((sourceData>>7)&0x7F); // high 7bits
			WriteByteHDMITXP0(writeAddr, writeLowData);
			writeAddr++;
			WriteByteHDMITXP0(writeAddr, writeHighData);
			writeAddr++;

			// set CbCr Output Offset
			sourceData = *sourcePtr; sourcePtr++;
			writeLowData = (BYTE)(sourceData&0x7F); // low 7bits
			writeHighData = (BYTE)((sourceData>>7)&0x7F); // high 7bits
			WriteByteHDMITXP0(writeAddr, writeLowData);
			writeAddr++;
			WriteByteHDMITXP0(writeAddr, writeHighData);
			writeAddr++;
		}
		WriteByteHDMITXP0(0x50 , data[1]);
	}
	else{
		// xvYCC conversion function is disable.
		WriteByteHDMITXP0(0x50 , data[1]);
	}
}
*/
#if SYSTEM_USE_HTX_HDCP == 1
//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for HDCP-Auth Handler
//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowInfoRiFailure(BYTE info)
{
#if __MDINHTX_DBGPRT__ == 1
	if		(info&0x10)	UARTprintf("RI_READING_MORE_ONE_FRAME\n");
	else if (info&0x80)	UARTprintf("RI_MISS_MATCH_LAST_FRAME\n");
	else if (info&0x40)	UARTprintf("RI_MISS_MATCH_FIRST_FRAME\n");
	else if (info&0x20)	UARTprintf("MASK_RI_NOT_CHANGED\n");

	UARTprintf("Ri interrupt asserted %d times\n", ++GetFAIL);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowInfoAuthState(PMDIN_HDMICTRL_INFO pCTL)
{
#if __MDINHTX_DBGPRT__ == 1
	if (OldAUTH==pCTL->auth||pCTL->auth==HDCP_AUTHEN_BGN) return;

	UARTprintf("{HDMI-Auth}: ");
	switch (pCTL->auth) {
		case HDCP_NOT_DEVICE:	UARTprintf("Sink is not HDCP\n"); break;
		case HDCP_BKSV_ERROR:	UARTprintf("BKSV Error\n"); break;
		case HDCP_R0s_NOSAME:	UARTprintf("R0 mis-match\n"); break;
		case HDCP_AUTHEN_END:	UARTprintf("Authenticated\n"); break;
		case HDCP_REPEAT_REQ:	UARTprintf("RPT-Auth. Req\n"); break;
		case HDCP_SHACAL_REQ:	UARTprintf("SHA calc. Req\n"); break;
		case HDCP_VHx_NOSAME:	UARTprintf("VHx Error\n"); break;
		case HDCP_CAS_EXCEED:	UARTprintf("MAX_CASCADE_EXCEEDED\n"); break;
		case HDCP_MAX_EXCEED:	UARTprintf("MAX_DEVS_EXCEEDED\n"); break;
		case HDCP_DEV_EXCEED:	UARTprintf("NUM_DEVS_EXCEEDED\n"); break;
		case HDCP_REAUTH_REQ:	UARTprintf("RE-Authen Req\n"); break;
		case HDCP_AUTHEN_REQ:	UARTprintf("Authentication Req\n"); break;
		default:				UARTprintf("Unknown %02d\n", pCTL->auth); break;
	}
	OldAUTH = pCTL->auth;
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_WriteHDCPRX(WORD rAddr, PBYTE pBuff, BYTE bytes)
{
	MDIN_HDMIMDDC_INFO stMDDC;

	stMDDC.sAddr		= MAKEWORD(0x74, 0);
	stMDDC.rAddr		= MAKEWORD(rAddr,0);
	stMDDC.bytes		= bytes;
	stMDDC.cmd			= 0x06;		// sequential write
	stMDDC.pBuff		= pBuff;
	return MDINHTX_WriteMDDC(&stMDDC);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ReadHDCPRX(WORD rAddr, PBYTE pBuff, BYTE bytes)
{
	MDIN_HDMIMDDC_INFO stMDDC;

	stMDDC.sAddr		= MAKEWORD(0x74, 0);
	stMDDC.rAddr		= MAKEWORD(rAddr,0);
	stMDDC.bytes		= bytes;
	stMDDC.cmd			= 0x02;		// sequential read
	stMDDC.pBuff		= pBuff;
	return MDINHTX_ReadMDDC(&stMDDC);
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_WriteHDCPTX(WORD rAddr, PBYTE pBuff, BYTE bytes)
{
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, rAddr, pBuff, bytes)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_ReadHDCPTX(WORD rAddr, PBYTE pBuff, BYTE bytes)
{
	if (MDINHIF_MultiRead(MDIN_HDMI_ID, rAddr, pBuff, bytes)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetKSVCmd(BYTE rAddr, WORD bytes)
{
	MDIN_HDMIMDDC_INFO stMDDC;

	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0f00)) return MDIN_I2C_ERROR;	// ABORT
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0900)) return MDIN_I2C_ERROR;	// CLEAR

	stMDDC.sAddr		= MAKEWORD(0x74, 0);
	stMDDC.rAddr		= MAKEWORD(rAddr,0);
	stMDDC.bytes		= bytes;
	stMDDC.cmd			= 0x02;		// sequential read

	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x0ec, (PBYTE)&stMDDC, 6)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, MAKEWORD(stMDDC.cmd,0))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetFIFOCntDone(WORD bytes)
{
	WORD rVal = 0, count = 500;

	while (count&&(rVal<bytes)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0f5, &rVal)) return MDIN_I2C_ERROR;
		count--;	MDINDLY_mSec(1);	// delay 1ms
	}

#if __MDINHTX_DBGPRT__ == 1
	if (count==0) UARTprintf("MDINHTX_GetFIFOCntDone %d BYTEs failed\n", bytes);
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetKSVFIFO(PBYTE pBuff, WORD bytes)
{
	WORD i, rVal;

	if (MDINHTX_GetFIFOCntDone(bytes)) return MDIN_NO_ERROR;

	for (i=0; i<bytes; i++) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0f4, &rVal)) return MDIN_I2C_ERROR;
		pBuff[i] = LOBYTE(rVal);
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetKSVFIFO(PBYTE pBuff, WORD bytes)
{
	WORD i;

	for (i=0; i<bytes; i++) {
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x0cc, 8, 8, pBuff[i])) return MDIN_I2C_ERROR;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_IsRXRepeater(void)
{
	if (MDINHTX_ReadHDCPRX(0x040, &GetRPT, 1)) return MDIN_I2C_ERROR;
	GetRPT = (GetRPT&0x40)? TRUE : FALSE;

#if __MDINHTX_DBGPRT__ == 1
	if (GetRPT)	UARTprintf("It is Repeater!!\n");
	else		UARTprintf("It is TV!!\n");
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_AreR0sMatch(void)
{
	WORD R0RX, R0TX;

	if (MDINHTX_ReadHDCPRX(0x008, (PBYTE)&R0RX, 2)) return MDIN_I2C_ERROR;
	if (MDINHTX_ReadHDCPTX(0x022, (PBYTE)&R0TX, 2)) return MDIN_I2C_ERROR;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("R0RX = 0x%04X, R0TX = 0x%04X\n", R0RX, R0TX);
#endif

	GetSAME = (R0RX==R0TX)? TRUE : FALSE;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_IsDeviceHDCP(void)
{
	BYTE i, j, ones = 0, rBuff[5];

	// read BKSV from RX
	if (MDINHTX_ReadHDCPRX(0x000, rBuff, 5)) return MDIN_I2C_ERROR;

	for (i=0; i<5; i++) {
		for (j=0; j<8; j++) {
			if (rBuff[i]&(1<<j)) ones++;
		}
	}

	GetHDCP = (ones==20)? TRUE : FALSE;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableEncryption(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 8, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableKSVReady(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 7, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 9, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	if (OnOff==OFF) return MDIN_NO_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 8, 1, ON)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_KSVReadyHandler(PMDIN_HDMICTRL_INFO pCTL, BYTE info)
{
	if (pCTL->auth!=HDCP_REPEAT_REQ||(info&0x80)==0) return MDIN_NO_ERROR;

	if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
	pCTL->auth = HDCP_SHACAL_REQ;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableRISCheck(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 12, 2, (OnOff)? 3 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 15, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026,  8, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_RISCheckHandler(PMDIN_HDMICTRL_INFO pCTL, BYTE info)
{
	if (pCTL->auth==HDCP_AUTHEN_BGN||(info&0xb0)==0) return MDIN_NO_ERROR;

	if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
	MDINHTX_ShowInfoRiFailure(info);	// debug print

	pCTL->auth = HDCP_REAUTH_REQ;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetHDCPAuthMode(PMDIN_HDMICTRL_INFO pCTL)
{
	WORD rVal, wBuff[5];	BYTE i, rBuff[10];

	// Those check must not be enabled before 1st part of Authentication.
	if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;

	if (MDINHTX_IsDeviceHDCP()) return MDIN_I2C_ERROR;
	if (GetHDCP==0) {pCTL->auth = HDCP_NOT_DEVICE; return MDIN_NO_ERROR;}

	// CP reset is normal operation
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 10, 1, 1)) return MDIN_I2C_ERROR;

	// set repeater of RX
	if (MDINHTX_IsRXRepeater()) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 12, 1, GetRPT)) return MDIN_I2C_ERROR;

	// generate AN
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 11, 1, 0)) return MDIN_I2C_ERROR;
	MDINDLY_mSec(10);
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 11, 1, 1)) return MDIN_I2C_ERROR;

	// read AN from HDCPTX
	if (MDINHTX_ReadHDCPTX(0x014, (PBYTE)wBuff, 10)) return MDIN_I2C_ERROR;
	for (i=0; i<5; i++) {	// conversion WORD-array to BYTE-array
		rBuff[i*2+0] = LOBYTE(wBuff[i]); rBuff[i*2+1] = HIBYTE(wBuff[i]);
	}

	// write AN to HDCPRX
	if (MDINHTX_WriteHDCPRX(0x018, rBuff+1, 8)) return MDIN_I2C_ERROR;

	// read AKSV from HDCPTX
	if (MDINHTX_ReadHDCPTX(0x01c, (PBYTE)wBuff, 6)) return MDIN_I2C_ERROR;
	for (i=0; i<3; i++) {	// conversion WORD-array to BYTE-array
		rBuff[i*2+0] = LOBYTE(wBuff[i]); rBuff[i*2+1] = HIBYTE(wBuff[i]);
	}

	// write AKSV to HDCPRX
	if (MDINHTX_WriteHDCPRX(0x010, rBuff+1, 5)) return MDIN_I2C_ERROR;

	// read BKSV from HDCPRX
	if (MDINHTX_ReadHDCPRX(0x000, rBuff, 5)) return MDIN_I2C_ERROR;
	for (i=0; i<3; i++) {	// conversion BYTE-array to WORD-array
		wBuff[i] = MAKEWORD(rBuff[i*2+1], rBuff[i*2+0]);
	}

	// Write BKSV to HDCPTX
	if (MDINHTX_WriteHDCPTX(0x010, wBuff, 4)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x014, 0, 8, rBuff[4])) return MDIN_I2C_ERROR;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x00e, &rVal)) return MDIN_I2C_ERROR;
	if (rVal&0x2000) {pCTL->auth = HDCP_BKSV_ERROR; return MDIN_NO_ERROR;}

	MDINDLY_mSec(200); // Delay for R0 calculation, Suppress R0 read before 100ms interval between BKSV write and R read

	if (MDINHTX_AreR0sMatch()) return MDIN_I2C_ERROR;
	if (GetSAME==0) {pCTL->auth = HDCP_R0s_NOSAME; return MDIN_NO_ERROR;}

	// set RiCheck interrupt mask
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x074, 10, 1, 1)) return MDIN_I2C_ERROR;

	if (MDINHTX_EnableControlPKT(OFF)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableEncryption(ON)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableRISCheck(ON)) return MDIN_I2C_ERROR;

	if (MDINHTX_IsRXRepeater()) return MDIN_I2C_ERROR;
	if (GetRPT==0) {pCTL->auth = HDCP_AUTHEN_END; return MDIN_NO_ERROR;}

	if (MDINHTX_EnableKSVReady(ON)) return MDIN_I2C_ERROR;

	pCTL->auth = HDCP_REPEAT_REQ;	GetFIFO = 5000;	// set 5sec time out
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_HDCPAuthHandler(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	if (pCTL->auth==HDCP_AUTHEN_BGN) return MDIN_NO_ERROR;

	MDINHTX_ShowInfoAuthState(pCTL);	// debug print

	if (pCTL->proc!=HTX_CABLE_HDMI_OUT&&
		pCTL->proc!=HTX_CABLE_DVI_OUT) return MDIN_NO_ERROR;

	if (pCTL->auth==HDCP_AUTHEN_END) return MDIN_NO_ERROR;
	if (pCTL->auth==HDCP_SHACAL_REQ) return MDIN_NO_ERROR;

	if (pCTL->auth==HDCP_REPEAT_REQ) {
		if (GetFIFO) GetFIFO--;	MDINDLY_mSec(1);
		if (GetFIFO) return MDIN_NO_ERROR;
	}

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("ReAthentication begin!!!\n");
#endif

	if (GetHDMI==FALSE) {		// check sink is HDMI
		if (MDINHTX_InitModeDVI(pINFO)) return MDIN_I2C_ERROR;

#if __MDINHTX_DBGPRT__ == 1
		UARTprintf("[HDCP.C](ReAthentication): Call InitDVITX() #1\n");
#endif
	}

	// Must turn encryption off when AVMUTE
	if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;

	if (GetHDMI==FALSE) {		// check sink is HDMI
		if (MDINHTX_InitModeDVI(pINFO)) return MDIN_I2C_ERROR;

#if __MDINHTX_DBGPRT__ == 1
		UARTprintf("[HDCP.C](ReAthentication): Call InitDVITX() #2\n");
#endif
	}

	if (MDINHTX_SetHDCPAuthMode(pCTL)) return MDIN_I2C_ERROR;
	MDINHTX_ShowInfoAuthState(pCTL);	// debug print

	if (pCTL->auth==HDCP_AUTHEN_END) return MDIN_NO_ERROR;
	if (pCTL->auth==HDCP_REPEAT_REQ) return MDIN_NO_ERROR;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("Authentication is failure. ReAuthentication.\n");
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for HDCP-SHA Handler
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_GetSHAProcDone(void)
{
	WORD rVal = 0, count = 100;

	while (count&&(rVal==0x00)) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x0cc, &rVal)) return MDIN_I2C_ERROR;
		rVal &= 0x01; count--;	MDINDLY_10uSec(10);		// delay 100us
	}
	if (count) MDINDLY_mSec(1);		// delay 1ms

#if __MDINHTX_DBGPRT__ == 1
	if (count==0) UARTprintf("MDINHTX_GetSHAProcDone() TimeOut Error!!!\n");
#endif

	return (count)? MDIN_NO_ERROR : MDIN_TIMEOUT_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SHATransform(BYTE num)
{
	BYTE i, rBuff[5]; WORD bytes = num*5;

	// set KSV length
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0ca, bytes)) return MDIN_I2C_ERROR;

	// clear SHA_GONE
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x0cc, 0, 8, 0x0a)) return MDIN_I2C_ERROR;

	// set SHA_START 
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x0cc, 0, 8, 0x09)) return MDIN_I2C_ERROR;

	// get KSV list from HDCPRX & set KSV FIFO to HDCPTX
	if (MDINHTX_SetKSVCmd(0x43, bytes)) return MDIN_I2C_ERROR;

	for (i=0; i<num; i++) {
		if (MDINHTX_GetKSVFIFO(rBuff, 5)) return MDIN_I2C_ERROR;
		if (MDINHTX_SetKSVFIFO(rBuff, 5)) return MDIN_I2C_ERROR;
	}

	return MDINHTX_GetSHAProcDone();
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SHACompareVi(void)
{
	BYTE i, j, rBuff[4], vBuff[4];

	if (MDINHTX_SetKSVCmd(0x20, 20)) return MDIN_I2C_ERROR;

	for (i=0; i<5; i++) {
		if (MDINHTX_GetKSVFIFO(rBuff, 4)) return MDIN_I2C_ERROR;
		if (MDINHTX_ReadHDCPTX(0x0d8+i*4, vBuff, 4)) return MDIN_I2C_ERROR;

		for (j=0; j<4; j++) {
			if (rBuff[j]!=vBuff[j]) return MDIN_SHA_CMP_ERROR;
		}
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_HDCPSHAHandler(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;
	MDIN_ERROR_t err;	BYTE rBuff[2];	WORD wBuff[2];

	if (pCTL->auth!=HDCP_SHACAL_REQ) return MDIN_NO_ERROR;

	if (MDINHTX_ReadHDCPRX(0x041, rBuff, 2)) return MDIN_I2C_ERROR;
	wBuff[0] = MAKEWORD(rBuff[1], rBuff[2]);
	if (MDINHTX_WriteHDCPTX(0x0ce, wBuff, 2)) return MDIN_I2C_ERROR;

	if (rBuff[1]&0x08) {
		pCTL->auth = HDCP_CAS_EXCEED;
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		MDINDLY_mSec(100); // To suppress 1st part auth start before RiCheck stop
		return MDIN_NO_ERROR;
	}
	if (rBuff[0]&0x80) {
		pCTL->auth = HDCP_MAX_EXCEED;
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		MDINDLY_mSec(100); // To suppress 1st part auth start before RiCheck stop
		return MDIN_NO_ERROR;
	}
	if (rBuff[0]>12) {
		pCTL->auth = HDCP_DEV_EXCEED;
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		MDINDLY_mSec(100); // To suppress 1st part auth start before RiCheck stop
		return MDIN_NO_ERROR;
	}

	err = MDINHTX_SHATransform(rBuff[0]); if (err==MDIN_I2C_ERROR) return err;
	if (err==MDIN_TIMEOUT_ERROR) {
		pCTL->auth = HDCP_FIFO_ERROR;
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		MDINDLY_mSec(100); // To suppress 1st part auth start before RiCheck stop
		return MDIN_NO_ERROR;
	}

	err = MDINHTX_SHACompareVi(); if (err==MDIN_I2C_ERROR) return err;
	if (err==MDIN_SHA_CMP_ERROR) {
		pCTL->auth = HDCP_VHx_NOSAME;
		if (MDINHTX_EnableRISCheck(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableKSVReady(OFF)) return MDIN_I2C_ERROR;
		if (MDINHTX_EnableEncryption(OFF)) return MDIN_I2C_ERROR;
		MDINDLY_mSec(100); // To suppress 1st part auth start before RiCheck stop
		return MDIN_NO_ERROR;
	}

	pCTL->auth = HDCP_AUTHEN_END;
	if (MDINHTX_EnableRISCheck(ON)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}
#endif	/* SYSTEM_USE_HTX_HDCP == 1 */

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for ETC (reset, video, audio, etc)
//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetModeHDMI(BOOL OnOff)
{
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x12e, 8, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	GetHDMI = MBIT(OnOff,1);
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static BOOL			MDINHTX_IsModeHDMI(void)
{
	return (GetHDMI)? TRUE : FALSE;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_Set656Mode(BYTE mode)
{
	WORD rVal = defMDINHTXVideo[mode].stB656.i_adj;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x03e, 0, 8, LOBYTE(rVal))) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stB656.h_syn;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x040, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stB656.o_fid;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x042, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stB656.h_len;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x044, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stB656.v_syn;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x046, 0, 8, LOBYTE(rVal))) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stB656.v_len;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x046, 8, 8, LOBYTE(rVal))) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stWIND.x;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x032, 0, 12, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stWIND.y;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x034, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stWIND.w;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x036, rVal)) return MDIN_I2C_ERROR;

	rVal = defMDINHTXVideo[mode].stWIND.h;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x038, rVal)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnableAllPWR(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	// set power down total
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x13c, 8, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	// disable Ri check
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 12, 2, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 15, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026,  8, 1, 0)) return MDIN_I2C_ERROR;

	// disable KSV ready
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 7, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 9, 1, 0)) return MDIN_I2C_ERROR;

	// disable encryption
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 8, 1, 0)) return MDIN_I2C_ERROR;

	if (OnOff==OFF) pCTL->proc = HTX_CABLE_PLUG_OUT;
	if (OnOff==OFF) pCTL->auth = HDCP_AUTHEN_BGN;

#if SYSTEM_USE_HTX_HDCP == 1
	if (OnOff==ON)  pCTL->auth = HDCP_REAUTH_REQ;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_EnablePhyPWR(PMDIN_VIDEO_INFO pINFO, BOOL OnOff)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	// disable Ri check
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 12, 2, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 15, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026,  8, 1, 0)) return MDIN_I2C_ERROR;

	// disable KSV ready
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x076, 7, 1, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 9, 1, 0)) return MDIN_I2C_ERROR;

	// disable encryption
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 8, 1, 0)) return MDIN_I2C_ERROR;

	// set power down mode
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x008, 0, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;

	if (OnOff==OFF) pCTL->auth = HDCP_AUTHEN_BGN;

#if SYSTEM_USE_HTX_HDCP == 1
	if (OnOff==OFF) return MDIN_NO_ERROR;
	pCTL->auth = (pCTL->proc==HTX_CABLE_HDMI_OUT)? HDCP_REAUTH_REQ: HDCP_AUTHEN_BGN;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SoftReset(PMDIN_VIDEO_INFO pINFO)
{
	if (MDINHTX_EnableControlPKT(ON)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnablePhyPWR(pINFO, OFF)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x004, 8, 8, 3)) return MDIN_I2C_ERROR;
	MDINDLY_mSec(1);
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x004, 8, 8, 0)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnablePhyPWR(pINFO, ON)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableControlPKT(OFF)) return MDIN_I2C_ERROR;
	MDINDLY_mSec(64);          // allow TCLK (sent to Rx across the HDMS link) to stabilize
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetVideoMode(PMDIN_VIDEO_INFO pINFO)
{
	WORD mode = 0, acen = 0;
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	if		(pINFO->stOUT_m.mode==MDIN_OUT_RGB444_8) {
		switch (pINFO->stVID_h.mode) {
			case HDMI_OUT_RGB444_8:	mode = 0x00; acen = 0x00; break;	// bypass / bypass
			case HDMI_OUT_YUV444_8: mode = 0x20; acen = 0x06; break;	// dither / RGB2YUV
			case HDMI_OUT_SEP422_8: mode = 0x20; acen = 0x07; break;	// dither / RGB2YUV,422
		}
	}
	else if (pINFO->stOUT_m.mode==MDIN_OUT_YUV444_8) {
		switch (pINFO->stVID_h.mode) {
			case HDMI_OUT_RGB444_8:	mode = 0x38; acen = 0x00; break;	// dither,YUV2RGB / bypass
			case HDMI_OUT_YUV444_8:	mode = 0x00; acen = 0x00; break;	// bypass / bypass
			case HDMI_OUT_SEP422_8: mode = 0x20; acen = 0x01; break;	// dither / 422
		}
	}
	else 											 {
		switch (pINFO->stVID_h.mode) {
			case HDMI_OUT_RGB444_8:	mode = 0x3c; acen = 0x00; break;	// dither,YUV2RGB,444 / bypass
			case HDMI_OUT_YUV444_8:	mode = 0x04; acen = 0x00; break;	// 444 / bypass
			case HDMI_OUT_SEP422_8: mode = 0x20; acen = 0x00; break;	// dither / bypass
		}
	}

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x04a, 0, 6, mode)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x048, 8, 3, acen)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x048, 0, 8, 0x00)) return MDIN_I2C_ERROR;
/*
	if (pINFO->dacPATH!=DAC_PATH_AUX_4CH&&pINFO->dacPATH!=DAC_PATH_AUX_2HD) return MDIN_NO_ERROR;

	// vid_mode - syncext for 4-CH input mode, 2-HD input mode
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x04a, 0, 1, 1)) return MDIN_I2C_ERROR;	// fix syncext
*/
	// de_ctrl - de_gen & vs_pol# & hs_pol#
	mode  = (pINFO->stOUT_m.stATTB.attb&MDIN_SCANTYPE_PROG)?  (0<<2) : (1<<2);
	mode |= (pINFO->stOUT_m.stATTB.attb&MDIN_POSITIVE_VSYNC)? (0<<1) : (1<<1);
	mode |= (pINFO->stOUT_m.stATTB.attb&MDIN_POSITIVE_HSYNC)? (0<<0) : (1<<0);
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x032, 12, 3, mode)) return MDIN_I2C_ERROR;

	// set embedded sync decoding & de generator registers
	if (MDINHTX_Set656Mode(pINFO->stOUT_m.frmt)) return MDIN_I2C_ERROR;

	// vid_mode - syncext for hdmi-bug
	mode = (pCTL->proc==HTX_CABLE_HDMI_OUT||pCTL->proc==HTX_CABLE_DVI_OUT)? 1 : 0;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x04a, 0, 1, mode)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetClockEdge(PMDIN_VIDEO_INFO pINFO)
{
	BOOL OnOff = MBIT(pINFO->stVID_h.fine, HDMI_CLK_EDGE_RISE);

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x008, 1, 1, MBIT(OnOff,1))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetDeepColor(PMDIN_VIDEO_INFO pINFO)
{
	BOOL OnOff = MBIT(pINFO->stVID_h.fine, HDMI_DEEP_COLOR_ON);

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x048, 14, 2, (OnOff)? 2 : 1)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x04a,  6, 2, (OnOff)? 2 : 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x04a,  5, 1, (OnOff)? 1 : 0)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x12e, 11, 3, (OnOff)? 6 : 4)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x12e, 14, 1, (OnOff)? 1 : 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetVideoPath(PMDIN_VIDEO_INFO pINFO)
{
	WORD rVal;

	if (MDINHTX_SetVideoMode(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetClockEdge(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetDeepColor(pINFO)) return MDIN_I2C_ERROR;

	// save packet buffer control registers
	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;

	// Reset internal state machines and allow TCLK to Rx to stabilize
	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;

	// Retrieve packet buffer control registers
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x13e, rVal)) return MDIN_I2C_ERROR;

	// set ICLK to not replicated
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x048, 0, 2, 0)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetSoftNVAL(PMDIN_HTXAUDIO_INFO pAUD)
{
	WORD rVal; DWORD nVal;

	if (pAUD->frmt==AUDIO_INPUT_SPDIF) {
		if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x118, &rVal)) return MDIN_I2C_ERROR;
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 0, 8, 3)) return MDIN_I2C_ERROR;
		pAUD->freq &= 0xf0; pAUD->freq |= LO4BIT(rVal);
	}

	switch (LO4BIT(pAUD->freq)) {
		case AUDIO_FREQ_32kHz:	nVal =  4096; break;
		case AUDIO_FREQ_44kHz:	nVal =  6272; break;
		case AUDIO_FREQ_48kHz:	nVal =  6144; break;
		case AUDIO_FREQ_88kHz:	nVal = 12544; break;
		case AUDIO_FREQ_96kHz:	nVal = 12288; break;
		case AUDIO_FREQ_176kHz:	nVal = 25088; break;
		case AUDIO_FREQ_192kHz:	nVal = 24576; break;
		default:				nVal =     0; break;
	}

	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x102, 8, 8, LOBYTE(LOWORD(nVal)))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x104, 0, 8, HIBYTE(LOWORD(nVal)))) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x104, 8, 8, LOBYTE(HIWORD(nVal)))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetAudioInit(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HTXAUDIO_INFO pAUD = (PMDIN_HTXAUDIO_INFO)&pINFO->stAUD_h;

	// enable SPDIF
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 0, 8, 3)) return MDIN_I2C_ERROR;

	// set software N_VAL
	if (MDINHTX_SetSoftNVAL(pAUD)) return MDIN_I2C_ERROR;

	// enable N/CTS packet
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x100, 8, 8, 2)) return MDIN_I2C_ERROR;

	// set MCLK
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x102, 0, 8, HI4BIT(pAUD->freq))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetAudioPath(PMDIN_VIDEO_INFO pINFO)
{
	WORD rVal, mode;
	PMDIN_HTXAUDIO_INFO pAUD = (PMDIN_HTXAUDIO_INFO)&pINFO->stAUD_h;

	// disable audio input stream
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 0, 1, OFF)) return MDIN_I2C_ERROR;

	// disable output audio packets
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00c, 9, 1, ON)) return MDIN_I2C_ERROR;

	// set software N_VAL
	if (MDINHTX_SetSoftNVAL(pAUD)) return MDIN_I2C_ERROR;

	// set software sampling frequency
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x120, 8, 8, LO4BIT(pAUD->freq))) return MDIN_I2C_ERROR;

	if (pAUD->frmt==AUDIO_INPUT_SPDIF) {
		// clear HDRA_ON, SCK_EDGE, CBIT_ORDER, VBIT
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x11c, 12, 4, 0)) return MDIN_I2C_ERROR;
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x11e,  0, 8, 0)) return MDIN_I2C_ERROR;

		// clear FS_OVERRIDE
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 9, 1, 0)) return MDIN_I2C_ERROR;

		MDINDLY_mSec(6);	// allow FIFO to flush

		// enable audio
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 0, 8, 3)) return MDIN_I2C_ERROR;

		// set LAYOUT_1
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x12e, 9, 2, 0)) return MDIN_I2C_ERROR;
	}
	else {
		// input I2S sample length
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x124, 0, 8, HIBYTE(pAUD->fine))) return MDIN_I2C_ERROR;
/*
		// set original sampling frequency
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x122, 4, 4, LO4BIT(pAUD->freq))) return MDIN_I2C_ERROR;

		// set audio sample word length
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x122, 0, 4, HIBYTE(pAUD->fine))) return MDIN_I2C_ERROR;
*/
		// set original sampling frequency & audio sample word length
		mode = (1<<12) | MAKEBYTE(LO4BIT(pAUD->freq), HIBYTE(pAUD->fine));
		if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x122, mode)) return MDIN_I2C_ERROR;

		// set I2S data in map register
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x11c, 0, 2, pAUD->frmt)) return MDIN_I2C_ERROR;

		// set I2S control register
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x11c, 8, 8, LOBYTE(pAUD->fine))) return MDIN_I2C_ERROR;

		// set FS_OVERRIDE
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 9, 1, 1)) return MDIN_I2C_ERROR;

		MDINDLY_mSec(6);	// allow FIFO to flush

		// enable audio
		mode = (pAUD->fine&AUDIO_MULTI_CHANNEL)? 15 : (1<<pAUD->frmt);
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x114, 0, 8, MAKEBYTE(mode,1))) return MDIN_I2C_ERROR;

		// set LAYOUT_1
		if (MDINHIF_RegField(MDIN_HDMI_ID, 0x12e, 9, 2, (mode==15)? 1 : 0)) return MDIN_I2C_ERROR;
	}

	// enable output audio packets
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00c, 9, 1, OFF)) return MDIN_I2C_ERROR;

	// save packet buffer control registers
	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x13e, &rVal)) return MDIN_I2C_ERROR;

	// Reset internal state machines and allow TCLK to Rx to stabilize
	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;

	// Retrieve packet buffer control registers
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x13e, rVal)) return MDIN_I2C_ERROR;

	// set MCLK
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x102, 0, 8, HI4BIT(pAUD->freq))) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_InitModeDVI(PMDIN_VIDEO_INFO pINFO)
{
#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("InitDVITX()!!!\n");
#endif

	//	FPLL is 1.0*IDCK.
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x082, 0, 8, 0x20)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetModeHDMI(OFF)) return MDIN_I2C_ERROR;

	// set wake-up
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x008, 0, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x078, 8, 8, 2)) return MDIN_I2C_ERROR;

	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;

//	if (MDINHTX_Set656Mode(pINFO->stOUT_m.frmt)) return MDIN_I2C_ERROR;

	pINFO->stVID_h.mode = HDMI_OUT_RGB444_8;	// Output must be RGB
	if (MDINHTX_SetVideoPath(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetAudioPath(pINFO)) return MDIN_I2C_ERROR;

	// Must be done AFTER setting up audio and video paths and BEFORE starting to send InfoFrames
	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;

	// disable encryption
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00c, 9, 2, 0)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 8, 1, 0)) return MDIN_I2C_ERROR;

	// AVI Mute clear, in DVI this must be used to clear mute
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x1de, 8, 8, 0x10)) return MDIN_I2C_ERROR;

	// Clear packet enable/repeat controls as they will not sent in DVI mode
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x13e, 0x0000)) return MDIN_I2C_ERROR;

	// set DVI interrupt mask
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x070, 0x4000)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x074, 0x4000)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_InitModeHDMI(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

#if __MDINHTX_DBGPRT__ == 1
	UARTprintf("InitHDMITX()!!!\n");
#endif

//	if (MDINHTX_Set656Mode(pINFO->stOUT_m.frmt)) return MDIN_I2C_ERROR;

	//	FPLL is 1.0*IDCK.
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x082, 0, 8, 0x20)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetModeHDMI(ON)) return MDIN_I2C_ERROR;

	// set wake-up
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x008, 0, 1, 1)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x078, 8, 8, 2)) return MDIN_I2C_ERROR;

	if (MDINHTX_SetAudioInit(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetVideoPath(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_SetAudioPath(pINFO)) return MDIN_I2C_ERROR;

	// Must be done AFTER setting up audio and video paths and BEFORE starting to send InfoFrames
	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;

	// enable AVI info-frame
	if (MDINHTX_EnableInfoFrmAVI(pINFO, ON)) return MDIN_I2C_ERROR;

	// disable encryption
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x00e, 8, 1, OFF)) return MDIN_I2C_ERROR;

	// enable AUD info-frame
	if (MDINHTX_EnableInfoFrmAUD(pINFO, ON)) return MDIN_I2C_ERROR;

	// Enable Interrupts: VSync, Ri check, HotPlug
	// CLR_MASK is BIT_INT_HOT_PLUG|BIT_BIPHASE_ERROR|BIT_DROP_SAMPLE
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x070, 0x5800)) return MDIN_I2C_ERROR;
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x074, 0x5800)) return MDIN_I2C_ERROR;
	
	pCTL->proc = HTX_CABLE_PLUG_OUT;
	pCTL->auth = HDCP_AUTHEN_BGN;

#if SYSTEM_USE_HTX_HDCP == 1
	pCTL->auth = HDCP_REAUTH_REQ;
#endif

#if SYSTEM_USE_HTX_HDCP == 0
	if (MDINHTX_EnableControlPKT(OFF)) return MDIN_I2C_ERROR; // Mute OFF
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static void MDINHTX_ShowStatusID(PMDIN_HDMICTRL_INFO pCTL)
{
#if __MDINHTX_DBGPRT__ == 1
	if (OldPROC==pCTL->proc) return;

	UARTprintf("{HDMI-Proc}: ");
	switch (pCTL->proc) {
		case HTX_CABLE_PLUG_OUT:	UARTprintf("PLUG Out\n");	break;
		case HTX_CABLE_EDID_CHK:	UARTprintf("check EDID\n");	break;
		case HTX_CABLE_HDMI_OUT:	UARTprintf("HDMI Out\n");	break;
		case HTX_CABLE_DVI_OUT:		UARTprintf("DVI Out\n");	break;
		default:					UARTprintf("Unknown\n");	break;
	}
	OldPROC = pCTL->proc;
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_IRQHandler(PMDIN_HDMICTRL_INFO pCTL)
{
	WORD rVal, rBuff[2];

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x070, &rVal)) return MDIN_I2C_ERROR;
	if ((rVal&0x01)==0) return MDIN_NO_ERROR;

	if (MDINHIF_MultiRead(MDIN_HDMI_ID, 0x070, (PBYTE)rBuff, 4)) return MDIN_I2C_ERROR;
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x070, (PBYTE)rBuff, 4)) return MDIN_I2C_ERROR;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x008, &rVal)) return MDIN_I2C_ERROR;

	// in order not to fail bouncing detection
	if ((rBuff[0]&0x4000)&&(rVal&0x0200)==0) pCTL->proc = HTX_CABLE_PLUG_OUT;

#if __MDINHTX_DBGPRT__ == 1
	if ((rBuff[0]&0x4000)&&(rVal&0x0200)==0)
		UARTprintf("HotPlug interrupt detection!!! = %d\n", rVal&0x0200);
#endif

	// Now clear all other interrupts
	if (MDINHIF_MultiWrite(MDIN_HDMI_ID, 0x070, (PBYTE)rBuff, 4)) return MDIN_I2C_ERROR;

#if SYSTEM_USE_HTX_HDCP == 1
	if (MDINHTX_KSVReadyHandler(pCTL, LOBYTE(rBuff[1]))) return MDIN_I2C_ERROR;
	if (MDINHTX_RISCheckHandler(pCTL, HIBYTE(rBuff[1]))) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_HPDHandler(PMDIN_HDMICTRL_INFO pCTL)
{
	WORD rVal;

	if (MDINHIF_RegRead(MDIN_HDMI_ID, 0x008, &rVal)) return MDIN_I2C_ERROR;

	if (rVal&0x0200) {
		if (pCTL->proc!=HTX_CABLE_PLUG_OUT) return MDIN_NO_ERROR;
		if (++GetPLUG<10) return MDIN_NO_ERROR;
		pCTL->proc = HTX_CABLE_EDID_CHK;
	}
	else {
		GetPLUG = 0; pCTL->proc = HTX_CABLE_PLUG_OUT;
//		CECDisconnect();
	}

#if SYSTEM_USE_HTX_HDCP == 1
	if ((rVal&0x0200)||pCTL->auth==HDCP_AUTHEN_BGN) return MDIN_NO_ERROR;
	pCTL->auth = HDCP_AUTHEN_REQ;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_PWRHandler(PMDIN_HDMICTRL_INFO pCTL)
{
	switch (pCTL->proc) {
		case HTX_CABLE_EDID_CHK: return MDINHIF_RegField(MDIN_HDMI_ID, 0x13c, 8, 1, 1);
		case HTX_CABLE_PLUG_OUT: return MDINHIF_RegField(MDIN_HDMI_ID, 0x13c, 8, 1, 0);
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetAUTOFormat(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;
	BYTE frmt;

	if ((pINFO->stVID_h.fine&HDMI_USE_AUTO_FRMT)==0) return MDIN_NO_ERROR;
	if ((pCTL->frmt==0x00)||(pCTL->frmt==0xff)) return MDIN_NO_ERROR;

	for (frmt=0; frmt<VIDOUT_FORMAT_END; frmt++) {
		if (pCTL->frmt==defMDINHTXVideo[frmt].stMODE.id_1) break;
		if (pCTL->frmt==defMDINHTXVideo[frmt].stMODE.id_2) break;
	}
	if (frmt==VIDOUT_FORMAT_END) return MDIN_NO_ERROR;

	// check call video process
	if (pINFO->stOUT_m.frmt==frmt)
		 pINFO->exeFLAG &= ~MDIN_UPDATE_HDMIFMT;	// not update
	else pINFO->exeFLAG |=  MDIN_UPDATE_HDMIFMT;	// update

	pINFO->stVID_h.mode = pCTL->mode;			// get native mode
	pINFO->stOUT_m.frmt = frmt;					// get native format
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_SetOutputMode(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	if (pCTL->proc==HTX_CABLE_HDMI_OUT) {
		if (MDINHTX_IsModeHDMI()==TRUE) return MDIN_NO_ERROR;
		if (MDINHTX_InitModeHDMI(pINFO)) return MDIN_I2C_ERROR;
	}

	if (pCTL->proc==HTX_CABLE_DVI_OUT) {
		if (MDINHTX_IsModeHDMI()==FALSE) return MDIN_NO_ERROR;
		if (MDINHTX_InitModeDVI(pINFO)) return MDIN_I2C_ERROR;
	}
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
static MDIN_ERROR_t MDINHTX_TimeOutHandler(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	if (GetEDID<10) return MDIN_NO_ERROR;
	pCTL->proc = HTX_CABLE_DVI_OUT;

	if (MDINHTX_SetOutputMode(pINFO)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
// Drive Function for HDMI-TX Handler, Video Process
//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINHTX_CtrlHandler(PMDIN_VIDEO_INFO pINFO)
{
	PMDIN_HDMICTRL_INFO pCTL = (PMDIN_HDMICTRL_INFO)&pINFO->stCTL_h;

	if (MDINHTX_IRQHandler(pCTL)) return MDIN_I2C_ERROR;	// IRQ handler
	if (MDINHTX_HPDHandler(pCTL)) return MDIN_I2C_ERROR;	// HPD handler
	if (MDINHTX_PWRHandler(pCTL)) return MDIN_I2C_ERROR;	// PWR handler

	MDINHTX_ShowStatusID(pCTL);	// debug print

	if (pCTL->proc==HTX_CABLE_EDID_CHK) {
		if (MDINHTX_GetParseEDID(pINFO)) return MDIN_I2C_ERROR;
		if (MDINHTX_VideoProcess(pINFO)) return MDIN_I2C_ERROR;
//		if (pCTL->proc!=HTX_CABLE_DVI_OUT) CECConnect();
		if (MDINHTX_SetAUTOFormat(pINFO)) return MDIN_I2C_ERROR;
		if (MDINHTX_SetOutputMode(pINFO)) return MDIN_I2C_ERROR;
	}

	if (MDINHTX_TimeOutHandler(pINFO)) return MDIN_I2C_ERROR;

//	if ((pCTL->proc!=HTX_CABLE_DVI_OUT)&&(pCTL->proc!=HTX_CABLE_PLUG_OUT))
//		CECHandler();

#if SYSTEM_USE_HTX_HDCP == 1
	if (MDINHTX_HDCPAuthHandler(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_HDCPSHAHandler(pINFO)) return MDIN_I2C_ERROR;
#endif

	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINHTX_VideoProcess(PMDIN_VIDEO_INFO pINFO)
{
	if (pINFO->stCTL_h.proc!=HTX_CABLE_HDMI_OUT&&
		pINFO->stCTL_h.proc!=HTX_CABLE_DVI_OUT) return MDIN_NO_ERROR;

	if (MDINHTX_SetVideoPath(pINFO)) return MDIN_I2C_ERROR;
	if (pINFO->stCTL_h.proc!=HTX_CABLE_HDMI_OUT) return MDIN_NO_ERROR;

	if (MDINHTX_SoftReset(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableInfoFrmAVI(pINFO, ON)) return MDIN_I2C_ERROR;
	if (MDINHTX_EnableInfoFrmAUD(pINFO, ON)) return MDIN_I2C_ERROR;
	return MDIN_NO_ERROR;
}

//--------------------------------------------------------------------------------------------------------------------------
MDIN_ERROR_t MDINHTX_SetHDMIBlock(PMDIN_VIDEO_INFO pINFO)
{
	if (MDINHTX_EnablePhyPWR(pINFO, ON)) return MDIN_I2C_ERROR;	// PhyPowerOn
	if (MDINHTX_EnableAllPWR(pINFO, ON)) return MDIN_I2C_ERROR;	// PowerOn

	// initialize MDDC
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0f00)) return MDIN_I2C_ERROR;	// ABORT
	MDINDLY_mSec(1);
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0900)) return MDIN_I2C_ERROR;	// CLEAR FIFO
	MDINDLY_mSec(1);
	if (MDINHIF_RegWrite(MDIN_HDMI_ID, 0x0f2, 0x0a00)) return MDIN_I2C_ERROR;	// CLOCK
	MDINDLY_mSec(1);
	if (MDINHIF_RegField(MDIN_HDMI_ID, 0x026, 8, 8, 0)) return MDIN_I2C_ERROR;	// disable Ri

//	if (MDINHTX_InitModeDVI(pINFO)) return MDIN_I2C_ERROR;
	if (MDINHTX_InitModeHDMI(pINFO)) return MDIN_I2C_ERROR;

//	CECInitialize();
	return MDIN_NO_ERROR;
}

#endif	/* defined(SYSTEM_USE_MDIN340)||defined(SYSTEM_USE_MDIN380) */

/*  FILE_END_HERE */
