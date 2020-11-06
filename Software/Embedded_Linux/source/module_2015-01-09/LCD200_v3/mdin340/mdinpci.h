//----------------------------------------------------------------------------------------------------------------------
// (C) Copyright 2010  Macro Image Technology Co., LTd. , All rights reserved
// 
// This source code is the property of Macro Image Technology and is provided
// pursuant to a Software License Agreement. This code's reuse and distribution
// without Macro Image Technology's permission is strictly limited by the confidential
// information provisions of the Software License Agreement.
//-----------------------------------------------------------------------------------------------------------------------
//
// File Name   		:  MDINPCI.H
// Description 		:  This file contains typedefine for the driver files	
// Ref. Docment		: 
// Revision History 	:

#ifndef		__MDINPCI_H__
#define		__MDINPCI_H__

// -----------------------------------------------------------------------------
// Include files
// -----------------------------------------------------------------------------
#ifndef		__MDINTYPE_H__
#include	"../Drivers/mdintype.h"
#endif

#if	defined(SYSTEM_USE_MDIN380)&&defined(SYSTEM_USE_PCI_HIF)
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
#define		MDINPCI_MHOST_OFFSET		0x00000000
#define		MDINPCI_LOCAL_OFFSET		0x00800000 
#define		MDINPCI_MHDMI_OFFSET		0x01000000
#define		MDINPCI_SDRAM_OFFSET		0x02000000

typedef struct
{ 
	DWORD	mmio_base;		// physical address of memory I/O	
	DWORD	mmio_virt;		// virtual address of memory I/O

}	stPACKED MDINPCI_MMIO, *PMDINPCI_MMIO;

#define		MDINPCI_DEV_NAME		"MDIN380-PCI"
#define		MDINPCI_DEV_MAJOR		240
#define		MDINPCI_DEV_INDEX		380
#define		MDINPCI_DEV_SIZE		(32*1024*1024)

typedef	struct {
	DWORD	addr;
	PDWORD	pBuff;

 }	stPACKED MDINPCI_STRUCT, *PMDINPCI_STRUCT;

#define		MDIN_IOCTL_MAGIC		'm'
#define		MDINPCI_REG_READ		_IOR(MDIN_IOCTL_MAGIC, 0, MDINPCI_STRUCT)
#define		MDINPCI_REG_WRITE		_IOW(MDIN_IOCTL_MAGIC, 1, MDINPCI_STRUCT)
#define		MDIN_IOCTL_MAXNR							   2

// -----------------------------------------------------------------------------
// Exported function Prototype
// -----------------------------------------------------------------------------

MDIN_ERROR_t MDINPCI_MultiWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINPCI_RegWrite(BYTE nID, DWORD rAddr, WORD wData);
MDIN_ERROR_t MDINPCI_MultiRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINPCI_RegRead(BYTE nID, DWORD rAddr, PWORD rData);
MDIN_ERROR_t MDINPCI_RegField(BYTE nID, DWORD rAddr, WORD bPos, WORD bCnt, WORD bData);

MDIN_ERROR_t MDINPCI_MMAPWrite(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);
MDIN_ERROR_t MDINPCI_MMAPRead(BYTE nID, DWORD rAddr, PBYTE pBuff, DWORD bytes);

#endif	/* SYSTEM_USE_PCI_HIF */


#endif	/* __MDINPCI_H__ */
