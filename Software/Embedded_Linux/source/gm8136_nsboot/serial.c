/*
 *	serial.c
 *
 *	Simple UART driver for 16550A.
 *
 *	Written by:     K.J. Lin <kjlin@faraday-tech.com>
 *
 *		Copyright (c) 2009 FARADAY, ALL Rights Reserve
 */
#include "board.h"

/* UART Channel */
#define	UART_BASE	REG_UART0_BASE

#define UART_RX		0x0		/* Receive buffer */
#define UART_TX		0x0		/* Transmit buffer */
#define UART_IER	0x4		/* Interrupt Enable Register */
#define UART_IIR	0x8		/* Interrupt Identification Register */
#define UART_FCR	0x8		/* FIFO Control Register */
#define UART_LCR	0xC		/* Line Control Register */
#define UART_MCR	0x10		/* Modem Control Register */
#define	UART_LSR	0x14		/* Line Status Register */
#define	UART_MSR	0x18		/* Modem Status Register */
#define	UART_DIV_L	0x0		/* [7:0] of the divisor (for 16x baud-rate generator) */
#define UART_DIV_H	0x4		/* [15:8] of the divisor */
#define UART_PSR	0x8		/* Prescale Divison Factor */

#define LSR_DR		0x01		/* Data ready */
#define LSR_OE		0x02		/* Overrun */
#define LSR_PE		0x04		/* Parity error */
#define LSR_FE		0x08		/* Framing error */
#define LSR_BI		0x10		/* Break */
#define LSR_THRE	0x20		/* Xmit holding register empty */
#define LSR_TEMT	0x40		/* Xmitter empty */
 

#define	readb(addr)	(*(volatile unsigned char *)(addr))
#define	readw(addr)	((*(volatile unsigned short *)(addr)))
#define readl(addr)	((*(volatile unsigned int *)(addr)))
#define writeb(b,addr)	(*(volatile unsigned char *)(addr)) = (b)
#define writew(b,addr)  (*(volatile unsigned short *)(addr)) = (b)
#define writel(b,addr)  (*(volatile unsigned int *)(addr)) = (b)

#define UART16550_READ(y)	(readl(UART_BASE + y) & 0xff)
#define UART16550_WRITE(y,z)	(writel(z&0xff, UART_BASE + y))
#define UART16550_WRITE_UART1(y,z)	(writel(z&0xff, REG_UART1_BASE + y))
#define UART16550_WRITE_UART5(y,z)	(writel(z&0xff, REG_UART5_BASE + y))

void serial_putc(unsigned char c)
{
	while ((UART16550_READ(UART_LSR) & LSR_THRE) == 0);
	UART16550_WRITE(UART_TX, c);
	return;
}

unsigned char serial_getc(void)
{
	while((UART16550_READ(UART_LSR) & LSR_DR) == 0);
	return UART16550_READ(UART_RX);
}

int serial_tstc(void)
{
	return((UART16550_READ(UART_LSR) & LSR_DR) != 0);
}

void serial_init(void)
{
    //UART0 baudrate 115200, clock source 25MHz
	UART16550_WRITE(UART_FCR, 0x35);
	UART16550_WRITE(UART_LCR, 0x83);
	UART16550_WRITE(UART_DIV_L, 0x0E);
	UART16550_WRITE(UART_DIV_H, 0x00);
	UART16550_WRITE(UART_LCR, 0x03);
	
	UART16550_WRITE(UART_TX, 0x30);
}
