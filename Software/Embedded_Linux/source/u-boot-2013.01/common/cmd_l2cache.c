/*
 * (C) Copyright 2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <config.h>
#include <asm/io.h>
#include <malloc.h>

/*************************************************************************************
 *  L2 cache IO Read/Write
 *************************************************************************************/
#define HSIZE_WORD      4
#define sw(x, y, z)     (*((unsigned int *)x) = y)

#define TEST_BUF_SIZE       (81920 * 4)
#define FTL2CC031_Base      0xE4B00000

#define AXI_S1_DDR_Base     0x8000000
#define AXI_S1_CB_Base      0x8100000   // Set to DDR address, DDR start from 0x00000000
#define AXI_S3_MMU_Base     0x9000000   // Set to DDR address

int do_l2cache_test (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int *cmp_buf;
    int i, value;

    cmp_buf = (unsigned int *)malloc(TEST_BUF_SIZE);
    if (cmp_buf == NULL) {
        printf(" --- test fail due to no memory! \n");
        return 1;
    }

    sw(AXI_S3_MMU_Base + 80*4, 0x8000c1e, HSIZE_WORD);    //CB
    sw(AXI_S3_MMU_Base + 81*4, 0x8100c1e,  HSIZE_WORD);  //CB
    sw(AXI_S3_MMU_Base + 90*4,  0x9000c12 , HSIZE_WORD);   // NCNV : S3 MMU -> 210*4 = 840
    sw(AXI_S3_MMU_Base + 0x392C, 0xE4B00c12 , HSIZE_WORD);   // NCNV : L2C base -> E4B*4 = 392C

    /* start to enable MMU and cache */
    //set r1 = TtbReg = 0x01000000;
    asm(
      "ldr r1,=0x9000000;"
      "mcr p15, 0, r1, c2, c0, 0;"
    );


    //set r1 = DomVal = 0x3;
    asm(
      "ldr r1,=0x3;"
      "mcr p15, 0, r1, c3, c0, 0;"
    );


    /* Enable L1 cache and mmu */
    // MCR/MRC p15, 0, Rd, c1, c0, 0   ; Write/Read CFG

    // Enable mmu
    asm(
      "mrc p15, 0, r1, c1, c0, 0;"
      "ldr r2,=0x1;"
      "orr r1,r1, r2;"
      "mcr p15, 0, r1, c1, c0, 0;"
    );


    // Enable I/D cache
    asm(
      "mrc p15, 0, r3, c1, c0, 0;"
      "ldr r2,=0x104;"
      "orr r3,r3, r2;"
      "mcr p15, 0, r3, c1, c0, 0;"
    );

    //set r1 = InvICa & InvDCa = 0x0;
    asm(
      "ldr r1,=0x0;"
      "mcr p15, 0, r1, c7, c7, 0;"
    );

    // Config Data ram 2T latch data
    sw(FTL2CC031_Base+0x8, 0x00010022 , HSIZE_WORD);
    //sw(FTL2CC031_Base+0x8, 0x21 , HSIZE_WORD);

    //clear err
    sw(FTL2CC031_Base+0x2c, 0xFFFFFFFF , HSIZE_WORD);

    /*3. Enable L2 cache. */
    sw(FTL2CC031_Base, 0x3 , HSIZE_WORD);

    /*4. write 256Kbyte to memory from 0x10_0000 to 0x10_FFFF. */ //16384  65536
    //for (i=0;i<256;i++)
    for (i=0;i<81920;i++)
    {
      sw(AXI_S1_CB_Base+0x4*i, i , HSIZE_WORD);
    }


    /*5. read 256Kbyte from memory from 0x10_0000 to 0x10_FFFF. */
    //for (i=0;i<256;i++)
    for (i=0;i<81920;i++)
    {
        value = *(unsigned int *)(AXI_S1_DDR_Base + 0x4 * i);
        if (value != i)
            printf("data[%d] = %d \n", i, value);
    }

    printf("The result of 0x20 is 0x%x \n", *(unsigned int *)(FTL2CC031_Base+0x20));

    for (;;)    {}

    return 0;
}

U_BOOT_CMD(
	l2cache_test, 1, 0,	do_l2cache_test,
	"Perform test of L2 cache",
	"Note: MMU will be enabled and never return."
);


