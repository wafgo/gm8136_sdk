//*****************************************************************************
//  Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.       *
//----------------------------------------------------------------------------*
// Name: spi_flash.h                                                          *
// Description: SPI flash access functions                                    *
//*****************************************************************************
#ifndef _SPI_FALSH_H_
#define _SPI_FALSH_H_

#include "board.h"

#define NOR_IMAGE_MAGIC         0x805A474D
#define SS_SIGNATURE            "GM8136"

#define READ_REG32(_register_)  *((volatile unsigned int *)(_register_))
#define WRITE_REG32(_register_, _value_)  (*((volatile unsigned int *)(_register_)) = (unsigned int)(_value_))

//=============================================================================
// System Header, size = 256 bytes
//=============================================================================
typedef struct 
{
    char signature[8];          /* Signature is "GM8xxx" */
    unsigned int bootm_addr;    /* Image offset to load by spiboot */
    unsigned int bootm_size;
    unsigned int bootm_reserved;
    
    struct {
        unsigned int addr;          /* image address */
        unsigned int size;          /* image size */
        unsigned char name[8];      /* image name */
        unsigned int reserved[1];
    } image[10];
    	
	struct 
	{
		unsigned int pagesz_log2;   // page size with bytes in log2
		unsigned int secsz_log2;    // sector size with bytes in log2
		unsigned int chipsz_log2;   // chip size with bytes in log2
		unsigned int clk_div;       // 2/4/6/8
		unsigned int reserved[4];   // unused
	} norfixup;	
	
	unsigned char   last_256[4];    // byte254:0x55, byte255:0xAA
} NOR_SYSHDR;

typedef struct 
{
    char signature[8];          /* Signature is "GM8xxx" */
    unsigned int bootm_addr;    /* Image offset to load by spiboot */
    unsigned int bootm_size;
    unsigned int bootm_reserved;
    
    struct {
        unsigned int addr;          /* image address */
        unsigned int size;          /* image size */
        unsigned char name[8];      /* image name */
        unsigned int reserved[1];
    } image[10];
    
    struct {
        unsigned int nand_numblks;      //number of blocks in chip
        unsigned int nand_numpgs_blk;   //how many pages in a block
        unsigned int nand_pagesz;       //real size in bytes                        
        unsigned int nand_sparesz_inpage;       //64bytes for 2k, ...... needed for NANDC023
        unsigned int nand_numce;        //how many CE in chip
        unsigned int nand_clkdiv;				// 2/4/6/8
        unsigned int nand_ecc_capability;   
        unsigned int nand_sparesz_insect;
    } nandfixup;
    unsigned int reserved2[64];        // unused
    unsigned char last_511[4];  // byte510:0x55, byte511:0xAA
} SPI_NAND_SYSHDR;

//=============================================================================
// SPI flash access functions 
//=============================================================================
void spi_load_para(INT32U dividor, INT32U pagesz_log2, 
                   INT32U secsz_log2, INT32U chipsz_log2);
INT32U spi_get_satus(void);
void spi_erase_sect(INT32U sector, INT32U sector_cnt);
void spi_erase_chip(void);
int spi_pgrd(INT32U page, INT32U *buf, INT32U len);
int spi_pgwr(INT32U page, INT32U *buf);

#endif //_SPI_FALSH_H_

//-----------------------------------------------------------------------------
// EOF spi_flash.h
