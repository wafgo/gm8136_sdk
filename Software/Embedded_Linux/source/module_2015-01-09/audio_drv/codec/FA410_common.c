//******************************************************************************
//
// Faraday Technology Coporation
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//******************************************************************************
// This file contains functions which set FA410 processor.

#include        "fa410.h"       // internal memory map definition

#define LINUX

extern int init_FA410_reg(void);

//******************************************************************************
//
// int SET_Data_Scratchpad_FA410(unsigned int, unsigned int)
//
//******************************************************************************
//
// input:
// parameter 1 -> data-scratchpad base address
// parameter 2 -> data-scratchpad size
// output: error message -> 0: ok, others: fail
//
// This function is used to set data-scratchpad for FA410 processor.
//
//******************************************************************************

int SET_Data_Scratchpad_FA410(unsigned int MPU_DScratchpad_base_address, unsigned int MPU_DScratchpad_size, unsigned int enable)
{
        unsigned int Rd, Rs = 0;
        //unsigned int data_scratchpad_base_address_mask = ~(0xFFFFFC00); //0x000003FF
        //unsigned int data_scratchpad_size_mask = ~(0xF0);  //0xFFFFFF0F
        //unsigned int data_scratchpad_enable_mask = ~(0x1); //0xFFFFFFFE


        MPU_DScratchpad_base_address >>= 10;

        Rd = (MPU_DScratchpad_base_address << 10) + (MPU_DScratchpad_size << 4) + (enable);

#ifdef LINUX
       asm volatile(
           "MRC  p15, 0, %[Rs], c11, c0, 0 "
           : [Rs]"+r"(Rs)
           :
           :"cc"
           );

       asm volatile(
           "AND     %[Rs], %[Rs], %[in01] \n "
           "AND     %[Rs], %[Rs], #0xFFFFFF0F \n"
           "AND     %[Rs], %[Rs], #0xFFFFFFFE \n"
           "ORR     %[Rd], %[Rs], %[Rd] "
           :[Rd]"+r"(Rd) , [Rs]"+r"(Rs)
           :[in01]"r"(0x000003FF)
           );

       asm volatile(
           "MCR     p15, 0, %[Rd], c11, c0, 0 "
            :
           	:[Rd]"r"(Rd)
           	:"cc"
            	 );

#else
        __asm
        {
           MRC     p15, 0, Rs, c11, c0, 0;
           AND     Rs, Rs, data_scratchpad_base_address_mask
           AND     Rs, Rs, data_scratchpad_size_mask
           AND     Rs, Rs, data_scratchpad_enable_mask
           ORR     Rd, Rs, Rd
           MCR     p15, 0, Rd, c11, c0, 0;
        }
#endif
        // richard add
#ifdef LINUX
       asm volatile(
            "MRC		p15, 0, %[Rs], c1, c0, 0"
             : [Rs]"+r"(Rs)
             :
             :"cc"
        );
#else
        __asm {
        		MRC		p15, 0, Rs, c1, c0, 0;
        		}
#endif

        Rd   =  ( Rs | 0x1 );
#ifdef LINUX
       asm volatile(
           "MCR     p15, 0, %[Rd], c1, c0, 0"
           :
       	   :[Rd]"r"(Rd)
       	   :"cc"
       );
#else
        __asm {
        		MCR     p15, 0, Rd, c1, c0, 0;
        		  }
#endif

        return 0;
}

//******************************************************************************
//
// int SET_IM_Base_Address_FA410(unsigned int)
//
//******************************************************************************
//
// input:
// parameter 1 -> IM base address
// output: error message -> 0: ok, others: fail
//
// This function is used to set IM base address for FA410 processor.
//
//******************************************************************************

int SET_IM_Base_Address_FA410(unsigned int IM_Base_address)
{
       unsigned int Rd =  IM_Base_address >> 18;


#ifdef LINUX
        asm volatile(
        "MCR     p15, 0, %[Rd], c12, c0, 0 "
        :
        :[Rd]"r"(Rd)
        :"cc"
        );
#else
        __asm
        {
            MCR     p15, 0, Rd, c12, c0, 0;
       }
#endif

       return 0;
}

//******************************************************************************
//
// int init_FA410_HW(void)
//
//******************************************************************************
//
// input: NA
// output: error message ->
// 0: ok
// 1: set data scratchpad fail
// 2: set IM base address fail
//
// This function is used to initial FA410 hardware including:
// 1) set data-scratchpad
// 2) set IM base address
//
//******************************************************************************

int init_FA410_HW(void)
{
        int err;
        unsigned int MPU_DScratchpad_size, MPU_DScratchpad_base_address;
        unsigned int IM_Base_address;


        MPU_DScratchpad_size = 6;                       // 64kBytes for FA410
        MPU_DScratchpad_base_address = IRAM_BASE;

        err = SET_Data_Scratchpad_FA410(MPU_DScratchpad_base_address, MPU_DScratchpad_size, 1);

        if (err != 0)
                return 1;

        IM_Base_address = MPU_DScratchpad_base_address; // IM base address is the same as the data-scratchpad base address

        err = SET_IM_Base_Address_FA410(IM_Base_address);

        if (err != 0)
                return 2;

        return 0;
}

//******************************************************************************
//
// int init_FA410(void)
//
//******************************************************************************
//
// input: NA
// output: error message -> 0: ok, 1: init_FA410_HW fail, 2: init_FA410_reg fail
//
// This function is used to initial FA410 processor including:
// 1) set data-scratchpad region
// 2) set IM base address
// 3) initail all co-processor registers as "0"
//
//******************************************************************************

int init_FA410(void)
{
        int err;

        err = init_FA410_HW();        // initial FA410 hardware

        if (err != 0)
                return 1;

	err = init_FA410_reg();       // initail FA410 co-processor registers

	if (err != 0)
                return 2;

        return 0;

}

//******************************************************************************
//
// End of file
//
//******************************************************************************
