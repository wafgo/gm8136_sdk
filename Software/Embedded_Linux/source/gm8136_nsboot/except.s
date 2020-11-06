;****************************************************************************
;* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
;*--------------------------------------------------------------------------*
;* Name:except.s                                                            *
;* Description: excpetion define                                            *
;* Author: Fred Chien                                                       *
;****************************************************************************


T_BIT		EQU	0x20
F_BIT		EQU	0x40
I_BIT		EQU	0x80
 
NoINTS		EQU	(I_BIT | F_BIT) 
MskINTS	EQU	NoINTS

AllIRQs		EQU	0xFF		; Mask for interrupt controller

ResetV		EQU	0x00
UndefV		EQU	0x04
SwiV		EQU	0x08
PrefetchV	EQU	0x0c
DataV		EQU	0x10	
IrqV		EQU	0x18
FiqV		EQU	0x1C

ModeMask	EQU	0x1F		; /* Processor mode in CPSR */

SVC32Mode	EQU	0x13
IRQ32Mode	EQU	0x12
FIQ32Mode	EQU	0x11
User32Mode	EQU	0x10
Sys32Mode	EQU	0x1F
;; /* Error modes */
Abort32Mode	EQU	0x17
Undef32Mode	EQU	0x1B


UserStackSize	EQU	0x10000
SVCStackSize	EQU	0x80000
IRQStackSize	EQU	0x20000
UndefStackSize	EQU	0x200
FIQStackSize	EQU	0x400
AbortStackSize	EQU	0x400

HeapSize        EQU 0x40000
 
SysSWI_Reason_EnterSVC  EQU 		0x17

	END				

