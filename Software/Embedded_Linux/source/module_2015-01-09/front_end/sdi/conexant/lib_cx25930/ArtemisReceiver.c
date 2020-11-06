/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2013 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "ArtemisReceiver.h"
#include "Comm.h"
#include "Cx25930Registers.h"

#define ARTM_LOS_BLACK_COLOR    1
#define ARTM_BLUE_FLD           1

/***********************************************************************************/
/* Forward declarations                                                            */
/***********************************************************************************/

//static unsigned char g_burst_transfer = 0;

/******************************************************************************
 *
 * ARTM_enableXtalOutput()

 *
 ******************************************************************************/
void
ARTM_enableXtalOutput(const CX_COMMUNICATION *p_comm)
{
    unsigned long value = 0;
    int result = TRUE;

    result &= readDword(p_comm, AMS_COMMON_TEST_DEBUG, &value);
    writeDword(p_comm, AMS_COMMON_TEST_DEBUG, value | 0x40);
}

/******************************************************************************
 *
 * ARTM_EQInit()
 *
 ******************************************************************************/
static void
ARTM_EQInit(const CX_COMMUNICATION *p_comm)
{
    unsigned long LINES_IN_FILE       = 1152;
    unsigned long EQ_LUT_BASE_ADDRESS = 0x2000;

#if 0    ///< Jerson
    int i = 0;

    if ( g_burst_transfer )
    {
        // Setup for burst transfers with auto-increment of 2.
        // Set number of bytes to burst as 0x1200. This is the MSB register.
        writeDword(p_comm, CSI_CONTROL_00, 0x0041);
        writeDword(p_comm, CSI_CONTROL_24, 0x0012);
    }

    /* Access lower half of the EQ LUT SRAM (bit 12), Enable EQ LUT SRAM */
    writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x0001);

    for (i = 0;  i < LINES_IN_FILE; i++ )
    {
        writeDword(p_comm, EQ_LUT_BASE_ADDRESS + (i * 2), EQ_LUT1[i]);
    }

    /* Access upper half of the EQ LUT SRAM (bit 12), Enable EQ LUT SRAM */
    writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x1001);

    for (i = 0;  i < LINES_IN_FILE; i++ )
    {
        writeDword(p_comm, EQ_LUT_BASE_ADDRESS + (i * 2), EQ_LUT2[i]);
    }
#else
    /* Access lower half of the EQ LUT SRAM (bit 12), Enable EQ LUT SRAM */
    writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x0001);

    burst_writeByte(p_comm, EQ_LUT_BASE_ADDRESS, (void *)EQ_LUT1, (int)(LINES_IN_FILE*2));

    /* Access upper half of the EQ LUT SRAM (bit 12), Enable EQ LUT SRAM */
    writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x1001);

    burst_writeByte(p_comm, EQ_LUT_BASE_ADDRESS, (void *)EQ_LUT2, (int)(LINES_IN_FILE*2));
#endif

    //printk("Artemis_EQInit: Done\n");
}

/*************************************************************************************************
// This function searches the optimal CDR charge pump current up/down value by stepping through
// the CDR_PH_UPCTLC/CDR_PH_DWNCTLC values, starting at 0x1F/0x0 => 0x0/0x0 => 0x0/0x1F, until the point where
// VCOGCALIB_TOO_FAST bit is clearled, then it writes that CDR_PH_UPCTLC/CDR_PH_DWNCTLC into registers
**************************************************************************************************/
static void ARTM_CdrCpCurrentUpDownCalibration(const CX_COMMUNICATION *p_comm)
{
    int ch_num;
    int vcogcalib_too_fast[4];
    int CdrCpUpCurrent     = 0xF;
    int CdrCpDownCurrent   = 0x0;

    for(ch_num=0; ch_num<4; ch_num++)
    {
        writeDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 0xF808);           // Lower CDR CP current to 1. and start searching from CDR_PH_UPCTLC/DWNCTLC = 0x1F/0x0
        WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 8, 7, 0x1);     // Change to lock to data mode => Set LOS_LOCK2REF_ENA = 0, and FORCE_LOCK2DATC = 1
        WriteBitField(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), 2, 2, 0x1);      // Hold EQ in reset while calibrating CDR Up/Down
    }

    p_comm->sleep(50);

    for(ch_num=0; ch_num<4; ch_num++)
        ReadBitField(p_comm, AMS_CH0_RXREG25 + (0x80*ch_num), 12, 12, (unsigned long *)&vcogcalib_too_fast[ch_num]);

    while(CdrCpUpCurrent > 0)
    {
        CdrCpUpCurrent--;

        for(ch_num=0; ch_num<4; ch_num++)
        {
            if(vcogcalib_too_fast[ch_num])
                WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, CdrCpUpCurrent);
        }

        p_comm->sleep(50);

        for(ch_num=0; ch_num<4; ch_num++)
        {
            if(vcogcalib_too_fast[ch_num])
                ReadBitField(p_comm, AMS_CH0_RXREG25 + (0x80*ch_num), 12, 12, (unsigned long *)&vcogcalib_too_fast[ch_num]);
        }
    }

    while(CdrCpDownCurrent < 0xF)
    {
        CdrCpDownCurrent++;

        for(ch_num=0; ch_num<4; ch_num++)
        {
            if(vcogcalib_too_fast[ch_num])
                WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, CdrCpDownCurrent);
        }

        p_comm->sleep(50);

        for(ch_num=0; ch_num<4; ch_num++)
        {
            if(vcogcalib_too_fast[ch_num])
                ReadBitField(p_comm, AMS_CH0_RXREG25 + (0x80*ch_num), 12, 12, (unsigned long *)&vcogcalib_too_fast[ch_num]);
        }
    }

    for(ch_num=0; ch_num<4; ch_num++ )
    {
        WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 8, 7, 0x2);  // Change to free running mode => Set LOS_LOCK2REF_ENA = 1, and FORCE_LOCK2DATC = 0
        WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 0x0);   // Remove EQ reset
        WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 5, 3, 0x3);  // Restore previous CDR charge pump current

        ReadBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 6, (unsigned long *)&CdrCpDownCurrent);
        CdrCpUpCurrent = CdrCpDownCurrent>>5;
        CdrCpDownCurrent = CdrCpDownCurrent & 0x1F;
        //printk("Ch %d: CdrCpUpCurrent = %d\tCdrCpDownCurrent=%d\n", ch_num, CdrCpUpCurrent,CdrCpDownCurrent);
    }
}

/******************************************************************************
 *
 * ARTM_GetTime()
// Return system time from the timestamp register
 *
*******************************************************************************/
long ARTM_GetTime(const CX_COMMUNICATION *p_comm, int ch_num, unsigned long StartTimestamp)
{
    unsigned long EndTimestamp=0, StartTime;
    long time;

    StartTime = StartTimestamp & 0xFFFF;

    readDword(p_comm, GC_TIMESTAMP, &EndTimestamp);

    if ( EndTimestamp < StartTime)
    {
        time = (0xFFFF + EndTimestamp - StartTime) * 10;
    }
    else
    {
        time = (EndTimestamp - StartTime) * 10;
    }

    return time;
}

/*************************************************************************************************
 *
 *  ARTM_CdrUpDnTest()
 *
 *************************************************************************************************/
int ARTM_CdrUpDnTest(const CX_COMMUNICATION *p_comm, int ch_num, int test)
{
    int ReLock=0, ReFail=0, ReStat, i=0;
    unsigned long StartTimestamp;
    int temp, dn, up;

    dn = ( test < 0 ) ? -test : 0;
    up = ( test < 0 ) ? 0 : test;

    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, up);
    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, dn);

    ARTM_disableEnableRE(p_comm, ch_num);

    readDword(p_comm, GC_TIMESTAMP, &StartTimestamp);

    temp = ARTM_GetTime(p_comm, ch_num, StartTimestamp);

    while( !ReLock && !ReFail && (temp < 150) )
    {
        ReadBitField(p_comm, RE0_DIAG_RCV_STAT0 + (0x1000*ch_num), 3, 0, (unsigned long *)&ReStat);
        ReLock = (ReStat == 0xF) ? 1 : 0;
        ReFail = (ReStat == 0x1) ? 1 : 0;

        //printk("ARTM_CdrUpDnTest.ReLock = %d\tARTM_CdrUpDnTest.ReFail = %d\n", ReLock, ReFail);

        i++;
        temp = ARTM_GetTime(p_comm, ch_num, StartTimestamp);

        if(i==300)
        {
            //printk("[%d]ARTM_GetTime in ARTM_CdrUpDnTest returned %d\n", i, temp);
            //printk("Stuck in ARTM_CdrUpDnTest...Breaking out...\n");
            break;
        }
    }

    //printk("ARTM_CdrUpDnTest:\n\tReLock = %d\n\tReFail = %d\n\tTime = %d\n", ReLock, ReFail, temp);

    return ReLock;
}

/*************************************************************************************************
 *
 *  ARTM_CdrCpCurrentUpDownSearch()
 *
 *************************************************************************************************/
int ARTM_CdrCpCurrentUpDownSearch(const CX_COMMUNICATION *p_comm, int ch_num, int InitCalVal)
{
    int cdr_down_value = 0;
    int cdr_up_value = 0;
    int result = 0;
    int MaxLock = -13;
    int MinLock = 13;
    int test;
    int i;
    int LockArray[6] = { 0, 0, 0, 0, 0, 0};
    int CdrMap[6][16] =
    {
        {  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
        { -6,  0,  0,  0,  0,  0,  0,  0, -6,  0,  0,  0,  0,  0,  0,  0},
        {  3,  0,  0,  0,  0,  0,  0,  0,  9,  0,  0,  0,  9,  0,  0,  0},
        { -3,  0, -3,  0, -9,  0, -9,  0,  0,  0,  0,  0, -9,  0, -9,  0},
        {  1,  0,  5,  5, -3, -3,  3,  3,  7,  7, 11, 11,  7,  7, 11, 11},
        { -1, -5,  0, -5, -7,-11, -7,-11,  3, -3,  3, -3, -7,-11, -7,-11}
    };

    //printk("Ch %d Optimizing CDR settings...\n", ch_num);

    for( i=0; i<6; i++ )
    {
        test = CdrMap[i][result];

        LockArray[5-i] = ARTM_CdrUpDnTest(p_comm, ch_num, test);
        result = 8*LockArray[5] + 4*LockArray[4] + 2*LockArray[3] + LockArray[2];

        MaxLock = ( (LockArray[5-i]) && (MaxLock < test) ) ? test : MaxLock;
        MinLock = ( (LockArray[5-i]) && (MinLock > test) ) ? test : MinLock;
    }

    //Fall back to initial calibration values if no good value is found
    if( !result )
    {
        cdr_up_value   = ( InitCalVal >= 0 ) ? InitCalVal : 0;
        cdr_down_value = ( InitCalVal <= 0 ) ? -InitCalVal : 0;

        //printk("Ch %d CDR Optimization Failed! Using calibration settings...\n", ch_num);
    }
    else
    {
        //printk("Setting CDR values in middle range...\n");

        //printk("MaxLock / MinLock = %d / %d\n", MaxLock, MinLock);

        MaxLock = (MaxLock + MinLock + 1) >> 1;
        cdr_up_value = ( MaxLock < 0 ) ? 0 : MaxLock;
        cdr_down_value = ( MaxLock < 0 ) ? -MaxLock : 0;
    }

    //printk("Ch %d CDR_PH_UPCTLC / DWNCTLC = %d / %d\n", ch_num, cdr_up_value, cdr_down_value);

    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, cdr_up_value);
    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, cdr_down_value);

    return (!result);
}

/******************************************************************************
 *
 * ARTM_InitReceiverChannel()
 *
 ******************************************************************************/
static void
ARTM_InitReceiverChannel(const CX_COMMUNICATION *p_comm, int ch_num)
{
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP) {
        writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0xD1FA);
        writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0x1402);
        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x0007);
        writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0x0008);

        writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 0xB840);
        writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 0x8840);

        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x005);
        writeDword(p_comm, AMS_CH0_RXREG12 + (0x80*ch_num), 0x8018);
        writeDword(p_comm, AMS_CH0_RXREG13 + (0x80*ch_num), 0x8018);
        writeDword(p_comm, AMS_CH0_RXREG15 + (0x80*ch_num), 0x8080);
        writeDword(p_comm, AMS_CH0_RXREG16 + (0x80*ch_num), 0x0080);

        /* Take EQ control out of soft reset */
        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x0001);
    }
    else {
        unsigned long temp;

        writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0xD1FA);        // <- A1 is set to 0x0008
        writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0x1402);

        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x000F);        // Reset peak detector
        p_comm->sleep(1);
        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x0007);
        writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0x0008);


        writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 0xB940);        // <- A1 is set to 0xB940
        writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 0x8940);        // <- A1 is set to 0x8940
        writeDword(p_comm, AMS_CH0_RXREG25 + (0x80*ch_num), 0x500F);

        // JG: A1 Start Calibration
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 15, 9, 0);    // UPPER_TARGET_IN_3G
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 8, 2, 0);     // LOWER_TARGET_IN_3G
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 15, 9, 0);   // UPPER_TARGET_IN_HD
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 8, 2, 0);    // LOWER_TARGET_IN_HD
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 15, 9, 0);   // UPPER_TARGET_IN_SD
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 8, 2, 0);    // LOWER_TARGET_IN_SD

        WriteBitField(p_comm, AMS_CH0_RXREG18 +(0x80*ch_num), 11, 8, 0x0);  // Set LOS threshold to 0 to prevent LOS from holding internal logic at reset, which prevents EQ comparators calibration

        WriteBitField(p_comm, AMS_CH0_RXREG1 + (ch_num*0x80), 8, 4, 0x1F);  // Put EQ in power down for calibration
        writeDword(p_comm, AMS_CH0_RXREG7 +(0x80*ch_num), 0x0005);          // Take EQ comparator counter out of reset

        writeDword(p_comm, AMS_CH0_RXREG15 +(0x80*ch_num), 0x8080);         // Enable automatic data & upper comparator calibration
        writeDword(p_comm, AMS_CH0_RXREG16 +(0x80*ch_num), 0x0080);         // Enable automatic EQ lower compactor calibration
        writeDword(p_comm, AMS_CH0_RXREG28 + (ch_num*0x80), 0);             // S et tracking ranges to 0 in case CTLE locks before we can read calibration values
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 1, 0, 0);     // Set tracking ranges to 0 in case CTLE locks before we can read calibration values
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 1, 0, 0);    // Set tracking ranges to 0 in case CTLE locks before we can read calibration values
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 1, 0, 0);    // Set tracking ranges to 0 in case CTLE locks before we can read calibration values
        WriteBitField(p_comm, AMS_CH0_RXREG7 +(0x80*ch_num), 2, 2, 0);      // Take CTLE CTRL out of reset to run calibration

        p_comm->sleep(5);

        readDword(p_comm, AMS_CH0_RXREG15 +(0x80*ch_num), &temp);           // Read calibration values
        temp = temp & 0x7F7F;                                               // Disable automatic data & upper comparator calibration

	// to avoid wrong calibrated value of CD Offset.
	if ((temp & 0x7F00) < 0x2000)
            temp = (temp & 0x007F) | 0x4000;

        writeDword(p_comm, AMS_CH0_RXREG15 +(0x80*ch_num), temp);
        writeDword(p_comm, AMS_CH0_RXREG15 +(0x80*ch_num), temp);

        readDword(p_comm, AMS_CH0_RXREG16 +(0x80*ch_num), &temp);           // Read calibration values
        temp = temp & 0xFF7F;                                               // Disable automatic lower comparator calibration

        writeDword(p_comm, AMS_CH0_RXREG16 +(0x80*ch_num), temp);
        writeDword(p_comm, AMS_CH0_RXREG16 +(0x80*ch_num), temp);

        WriteBitField(p_comm, AMS_CH0_RXREG18 +(0x80*ch_num), 11, 8, 0x3);

        writeDword(p_comm, AMS_CH0_RXREG5+(0x80*ch_num), 0x0010);           // Enable HPF Filter

        WriteBitField(p_comm, AMS_CH0_RXREG22 + (ch_num*0x80), 9, 4, 0x15); // Program CDR loop bandwidth control
        WriteBitField(p_comm, AMS_CH0_RXREG4 + (ch_num*0x80), 15, 12, 0x4); // BLW comp feedback amp ctrl EQ_BIAS_COMPBLW_CTLC = 0x4

        WriteBitField(p_comm, AMS_CH0_RXREG28 + (ch_num*0x80), 15, 0, 0x444);
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 1, 0, 0x2);
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 1, 0, 0x2);
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 1, 0, 0x2);

        //A1 Calibration Done

        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x005);
        writeDword(p_comm, AMS_CH0_RXREG12 + (0x80*ch_num), 0x8018);
        writeDword(p_comm, AMS_CH0_RXREG13 + (0x80*ch_num), 0x8018);

        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 15, 15, 0);    //For A1, Disable CDR HW Timeout

        /* Take EQ control out of soft reset */
        writeDword(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 0x0001);
    }
}

/******************************************************************************
 *
 * ARTM_RasterEngineEnable()
 *
 ******************************************************************************/
void
ARTM_RasterEngineEnable(const CX_COMMUNICATION *p_comm, int ch_num)
{
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, VOF_CSR_0 + (0x4*ch_num), 0x0017);
    else {
        writeDword(p_comm, VOF_CSR_0 + (0x4*ch_num), 0x0017);

        writeDword(p_comm, RE0_TRS_LOCK_THRESHOLD + (0x1000*ch_num), 0x4);

        if(ARTM_BLUE_FLD)
    	    writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F01);
        else
    	    writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x1F01);
    }
}

/******************************************************************************
 *
 * ARTM_ReceiverModeInit()
 *
 ******************************************************************************/
static void
ARTM_ReceiverModeInit(const CX_COMMUNICATION *p_comm)
{
    int i;
    int chip_rev = ARTM_get_Chip_RevID(p_comm);

    writeDword(p_comm, AMS_SYSPLL_CONTROL_HI, 0x0016);
    writeDword(p_comm, AMS_SYSPLL_CONTROL_HI, 0x0014);
    writeDword(p_comm, CRG_CLOCK_CONTROL, 0x0100);

    if(chip_rev == ARTM_CHIP_REV_11ZP) {
        ARTM_EQInit( p_comm );

        writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x1101);
    }
    else {
        writeDword(p_comm, AMS_INTF_SRAM_CTRL, 0x1102);

        for( i=0; i<4; i++)
            WriteBitField(p_comm, AMS_CH0_RXREG24 + (i*0x80), 2, 2, 0);
    }

    ARTM_InitReceiverChannel(p_comm, 0);
    ARTM_InitReceiverChannel(p_comm, 1);
    ARTM_InitReceiverChannel(p_comm, 2);
    ARTM_InitReceiverChannel(p_comm, 3);

    /* Turn on clocks to all 4 SDI channels. */
    writeDword(p_comm, CRG_CLOCK_CONTROL, 0x010F);
    p_comm->sleep(500);
    writeDword(p_comm, CRG_RESET_CONTROL, 0x3FFF);
    p_comm->sleep(500);

    ARTM_RasterEngineEnable(p_comm, 0);
    ARTM_RasterEngineEnable(p_comm, 1);
    ARTM_RasterEngineEnable(p_comm, 2);
    ARTM_RasterEngineEnable(p_comm, 3);
}

/******************************************************************************
 *
 * ARTM_load_RG59_0to50m_auto()
 *
 ******************************************************************************/
static void
ARTM_load_RG59_0to50m_auto(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value = 0;
    int result = TRUE;
    int chip_rev = ARTM_get_Chip_RevID(p_comm);

    writeDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 0x0018);
    writeDword(p_comm, AMS_CH0_RXREG23 + (0x80*ch_num), 0x5B80);
    if(chip_rev == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0x6403);
    else
        writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0xBC03);
    writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0x0000);
    writeDword(p_comm, AMS_CH0_RXREG2  + (0x80*ch_num), 0x000C);
    writeDword(p_comm, AMS_CH0_RXREG8  + (0x80*ch_num), 0x084C);

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value | 0x2000) );   // Enable BLW_ENA

    if(chip_rev == ARTM_CHIP_REV_11ZP) {
        result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
        writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0040) );  // manual rate

        result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
        writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0030) | 0x0020); //manual rate
    }

    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0010);           // Enable HPF Filter

    if(chip_rev != ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0000 );      // Disable HPF Filter

    result &= readDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), (value & ~0x0080) ); // Disable AUTO_DC_OFFSET_EN
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), 0x6740 );            // Set DC_OFFSET

    if(chip_rev == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0000 );          // Disable HPF Filter
    else {
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (0x10*ch_num), 8, 8, 1);
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (0x10*ch_num), 7, 0, 0x9F);
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (0x10*ch_num), 8, 8, 1);
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (0x10*ch_num), 7, 0, 0x4A);
    }

    writeDword(p_comm, AMS_CH0_TXREG1 + (0x10*ch_num), 0x4800 );          //Power down the TX channel
    writeDword(p_comm, AMS_CH0_TXREG3 + (0x10*ch_num), 0x002F );

    writeDword(p_comm, AMS_INTF_REGS_SPARE, 0x1001 );

    writeDword(p_comm, ANC0_INT_STAT, 0x0009 );
    writeDword(p_comm, GC_TIMESTAMP, 0x8CE4 );
}

/******************************************************************************

 *
 * ARTM_load_RG59_75to100m()
 *
 ******************************************************************************/
static void
ARTM_load_RG59_75to100m(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value = 0;
    int result = TRUE;

    writeDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 0x0018);
    writeDword(p_comm, AMS_CH0_RXREG23 + (0x80*ch_num), 0x5B80);
    writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0x6403);
    writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0x0000);
    writeDword(p_comm, AMS_CH0_RXREG2  + (0x80*ch_num), 0x000C);
    writeDword(p_comm, AMS_CH0_RXREG8  + (0x80*ch_num), 0x084C);

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value | 0x2000) );   // Enable BLW_ENA

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0040) );  // manual rate

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0030) | 0x0020); //manual rate

    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0010);           // Enable HPF Filter
    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0000);           // Disable HPF Filter

    result &= readDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), (value & ~0x0080) ); // Disable AUTO_DC_OFFSET_EN
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), 0x6740 );            // Set DC_OFFSET

    result &= readDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), value | 0x0100 );
    writeDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), (value & 0xFF00) | 0x009F );

    result &= readDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), value | 0x0100 );
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), (value & 0xFF00) | 0x004a );
    else
        writeDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), (value & 0xFF00) | 0x0042 );

    writeDword(p_comm, AMS_CH0_TXREG1 + (0x10*ch_num), 0x4800 );          //Power down the TX channel
    writeDword(p_comm, AMS_CH0_TXREG3 + (0x10*ch_num), 0x002F );

    writeDword(p_comm, AMS_INTF_REGS_SPARE, 0x1001 );

    writeDword(p_comm, ANC0_INT_STAT, 0x0009 );
    writeDword(p_comm, GC_TIMESTAMP, 0x8CE4 );
}

/******************************************************************************

 *

 * ARTM_load_RG59_125to150m()
 *
 ******************************************************************************/
static void
ARTM_load_RG59_125to150m(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value = 0;
    int result = TRUE;

    writeDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 0x0018);
    writeDword(p_comm, AMS_CH0_RXREG23 + (0x80*ch_num), 0x5B80);
    writeDword(p_comm, AMS_CH0_RXREG24 + (0x80*ch_num), 0x6403);
    writeDword(p_comm, AMS_CH0_RXREG1  + (0x80*ch_num), 0x0000);
    writeDword(p_comm, AMS_CH0_RXREG2  + (0x80*ch_num), 0x000C);
    writeDword(p_comm, AMS_CH0_RXREG8  + (0x80*ch_num), 0x084C);

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value | 0x2000) );   // Enable BLW_ENA

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0040) );  // manual rate

    result &= readDword(p_comm, AMS_CH0_RXREG8 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG8+(0x80*ch_num), (value & ~0x0030) | 0x0020); //manual rate

    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0010);           // Enable HPF Filter
    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0000);           // Disable HPF Filter

    result &= readDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), (value & ~0x0080) ); // Disable AUTO_DC_OFFSET_EN
    writeDword(p_comm, AMS_CH0_RXREG17+(0x80*ch_num), 0x6740 );            // Set DC_OFFSET

    result &= readDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), value | 0x0100 );
    writeDword(p_comm, AMS_CH0_RXREG13+(0x80*ch_num), (value & 0xFF00) | 0x009F );

    result &= readDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), value | 0x0100 );
    writeDword(p_comm, AMS_CH0_RXREG12+(0x80*ch_num), (value & 0xFF00) | 0x004a );

    writeDword(p_comm, AMS_CH0_TXREG1 + (0x10*ch_num), 0x4800 );          //Power down the TX channel
    writeDword(p_comm, AMS_CH0_TXREG3 + (0x10*ch_num), 0x002F );

    writeDword(p_comm, AMS_INTF_REGS_SPARE, 0x1001 );

    writeDword(p_comm, ANC0_INT_STAT, 0x0009 );
    writeDword(p_comm, GC_TIMESTAMP, 0x8CE4 );
}

/******************************************************************************
 *
 * ARTM_EQ_soft_reset()
 *
 ******************************************************************************/
static void
ARTM_EQ_soft_reset(const CX_COMMUNICATION *p_comm)
{
    unsigned long value = 0;
    int result = TRUE;

    result &= readDword(p_comm, AMS_CH0_RXREG7, &value);
    writeDword(p_comm, AMS_CH0_RXREG7, (value | 0x4) );

    result &= readDword(p_comm, AMS_CH1_RXREG7, &value);
    writeDword(p_comm, AMS_CH1_RXREG7, (value | 0x4) );

    result &= readDword(p_comm, AMS_CH2_RXREG7, &value);
    writeDword(p_comm, AMS_CH2_RXREG7, (value | 0x4) );

    result &= readDword(p_comm, AMS_CH3_RXREG7, &value);
    writeDword(p_comm, AMS_CH3_RXREG7, (value | 0x4) );

    result &= readDword(p_comm, AMS_CH0_RXREG7, &value);
    writeDword(p_comm, AMS_CH0_RXREG7, (value & ~0x4) );

    result &= readDword(p_comm, AMS_CH1_RXREG7, &value);
    writeDword(p_comm, AMS_CH1_RXREG7, (value & ~0x4) );

    result &= readDword(p_comm, AMS_CH2_RXREG7, &value);
    writeDword(p_comm, AMS_CH2_RXREG7, (value & ~0x4) );

    result &= readDword(p_comm, AMS_CH3_RXREG7, &value);
    writeDword(p_comm, AMS_CH3_RXREG7, (value & ~0x4) );

    p_comm->sleep(1000);
}

/******************************************************************************
 *
 * ARTM_CDR_soft_reset()
 *
 ******************************************************************************/
static void
ARTM_CDR_soft_reset(const CX_COMMUNICATION *p_comm)
{
    unsigned long value = 0;
    int result = TRUE;

    result &= readDword(p_comm, AMS_CH0_RXREG19, &value);
    writeDword(p_comm, AMS_CH0_RXREG19, (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH1_RXREG19, &value);
    writeDword(p_comm, AMS_CH1_RXREG19, (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH2_RXREG19, &value);
    writeDword(p_comm, AMS_CH2_RXREG19, (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH3_RXREG19, &value);
    writeDword(p_comm, AMS_CH3_RXREG19, (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH0_RXREG19, &value);
    writeDword(p_comm, AMS_CH0_RXREG19, (value & ~0x1000) );

    result &= readDword(p_comm, AMS_CH1_RXREG19, &value);
    writeDword(p_comm, AMS_CH1_RXREG19, (value & ~0x1000) );

    result &= readDword(p_comm, AMS_CH2_RXREG19, &value);
    writeDword(p_comm, AMS_CH2_RXREG19, (value & ~0x1000) );

    result &= readDword(p_comm, AMS_CH3_RXREG19, &value);
    writeDword(p_comm, AMS_CH3_RXREG19, (value & ~0x1000) );

    p_comm->sleep(1000);
}

/******************************************************************************
 *
 * ARTM_initEQ()
 * rate is input for manually set SD/HD/3G mode selection
 *
 * init before EQ N-time adaptation loop (ARTM_LoopInit(), ARTM_Loop())
 * *
 ******************************************************************************/
static int
ARTM_initEQ(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned long value = 0;
    int result = TRUE;

    result &= readDword(p_comm,  AMS_CH0_RXREG22 + (ch_num*0x80), &value);
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG22 + (ch_num*0x80), (value & ~0x03F0) | (0x9 << 4) );
    else
        writeDword(p_comm, AMS_CH0_RXREG22 + (ch_num*0x80), (value & ~0x03F0) | (0x1f << 4) );
    result &= readDword(p_comm, AMS_CH0_RXREG23 +(0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG23+(0x80*ch_num), value | 0x0008 );                       // CDR_DIV_OVERRIDEC = 1
    result &= readDword(p_comm, AMS_CH0_RXREG23 +(0x80*ch_num), &value);

    if(rate == ARTM_RATE_SD) {
        writeDword(p_comm, AMS_CH0_RXREG23+(0x80*ch_num), (value & ~0x0300) | (0x1 << 8) );   // CDR_FB_DIV_SELC = 0x1 3G Mode
    }else if(rate == ARTM_RATE_HD) {
        writeDword(p_comm, AMS_CH0_RXREG23+(0x80*ch_num), (value & ~0x0300) | (0x2 << 8) );   // CDR_FB_DIV_SELC = 0x2 HD Mode
    }else if(rate == ARTM_RATE_GEN3) {
        writeDword(p_comm, AMS_CH0_RXREG23+(0x80*ch_num), (value & ~0x0300) | (0x3 << 8) );   // CDR_FB_DIV_SELC = 0x3 3G Mode
    }
    result &= readDword(p_comm, AMS_CH0_RXREG24 +(0x80*ch_num), &value);
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP)
        writeDword(p_comm, AMS_CH0_RXREG24+(0x80*ch_num), (value & ~0xFC00) | (0x9 << 10) );  // CDR_INTCAP_CTLC = 0x9
    else
        writeDword(p_comm, AMS_CH0_RXREG24+(0x80*ch_num), (value & ~0xFC00) | (0x3 << 10) );  // CDR_INTCAP_CTLC = 0x9
    result &= readDword(p_comm, AMS_CH0_RXREG4 +(0x80*ch_num), &value);
    if(rate == ARTM_RATE_SD) {
        writeDword(p_comm, AMS_CH0_RXREG4+(0x80*ch_num), (value & ~0xF000) | (0x6 << 12) );   // BIAS_COMPBLW_CTLC
    }else {
        writeDword(p_comm, AMS_CH0_RXREG4+(0x80*ch_num), (value & ~0xF000) | (0x4 << 12) );   // BIAS_COMPBLW_CTLC
    }

    if(ARTM_get_Chip_RevID(p_comm) != ARTM_CHIP_REV_11ZP) {
        result &= readDword(p_comm, AMS_CH0_RXREG20 +(0x80*ch_num), &value);
        writeDword(p_comm, AMS_CH0_RXREG20+(0x80*ch_num), (value & ~0xEF00) | (0x7 << 13) | (0xf << 8) );   // CDR lock in_out PPM
    }

    return result;
}

/******************************************************************************
 *
 * ARTM_initialize()
 *
 ******************************************************************************/
int
ARTM_initialize(const CX_COMMUNICATION *p_comm, int length, int rate, int vout_fmt)
{
    unsigned long value = 0;
    int result = TRUE;
    int chip_rev = ARTM_get_Chip_RevID(p_comm);

    result &= readDword(p_comm, GC_CHIP_ID, &value);
    //printk("ARTM_initialize:  Device ID = 0x%x\n", (unsigned int)value);

    ARTM_ReceiverModeInit( p_comm );

    writeDword(p_comm, CRG_DEBUG_AUX, 0x1591 );

    if (  length > 1 ) // 125 to 150m
    {
        ARTM_load_RG59_125to150m( p_comm, 0 );
        ARTM_load_RG59_125to150m( p_comm, 1 );
        ARTM_load_RG59_125to150m( p_comm, 2 );
        ARTM_load_RG59_125to150m( p_comm, 3 );
    }
    else if (  length > 0 ) // 75 to 100m
    {
        ARTM_load_RG59_75to100m( p_comm, 0 );
        ARTM_load_RG59_75to100m( p_comm, 1 );
        ARTM_load_RG59_75to100m( p_comm, 2 );
        ARTM_load_RG59_75to100m( p_comm, 3 );
    }
    else
    {
        ARTM_load_RG59_0to50m_auto( p_comm, 0 );
        ARTM_load_RG59_0to50m_auto( p_comm, 1 );
        ARTM_load_RG59_0to50m_auto( p_comm, 2 );
        ARTM_load_RG59_0to50m_auto( p_comm, 3 );
    }

    /* EQ Soft Reset */
    ARTM_EQ_soft_reset( p_comm );

    /* CDR soft reset */
    ARTM_CDR_soft_reset( p_comm );

    /* EQ Soft Reset */
    ARTM_EQ_soft_reset( p_comm );

    if(chip_rev == ARTM_CHIP_REV_11ZP) {
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*0), 0x0000 );           // Disable HPF Filter
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*1), 0x0000 );           // Disable HPF Filter
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*2), 0x0000 );           // Disable HPF Filter
        writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*3), 0x0000 );           // Disable HPF Filter
    }

    ARTM_initEQ(p_comm, 0, rate);
    ARTM_initEQ(p_comm, 1, rate);
    ARTM_initEQ(p_comm, 2, rate);
    ARTM_initEQ(p_comm, 3, rate);

    if(chip_rev != ARTM_CHIP_REV_11ZP) {
        // CDR calibration
        ARTM_CdrCpCurrentUpDownCalibration(p_comm);
    }

    if(vout_fmt == ARTM_VOUT_FMT_BT1120)
        ARTM_enableBT1120(p_comm);
    else
        ARTM_enableBT656(p_comm);

    if(chip_rev != ARTM_CHIP_REV_11ZP)
        ARTM_Enable_GPIOC_MCLK(p_comm);

    //printk("ARTM_initialize: Done\n");

    return result;
}

/******************************************************************************
 *
 * ARTM_resetClock()

 *
 ******************************************************************************/
static void
ARTM_resetClock(const CX_COMMUNICATION *p_comm)
{
    writeDword(p_comm, CRG_CLOCK_CONTROL, 0x10F);

    writeDword(p_comm, CRG_RESET_CONTROL,     0x3FFF);
    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP) {
        writeDword(p_comm, GC_VID_OUT_CONFIG_A_B, 0x0100);
        writeDword(p_comm, GC_VID_OUT_CONFIG_C_D, 0x0001);
    }
    else {
        writeDword(p_comm, GC_VID_OUT_CONFIG_A_B, 0);    //for A1
        writeDword(p_comm, GC_VID_OUT_CONFIG_C_D, 0);
    }
}

/******************************************************************************
 *
 * ARTM_resetRECh()

 *
 ******************************************************************************/
static void
ARTM_resetRECh(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value = 0;
    int result = TRUE;

    result &= readDword(p_comm, CRG_RESET_CONTROL, &value);
    value = value & (~(1<<ch_num));
    writeDword(p_comm, CRG_RESET_CONTROL, value);

    p_comm->sleep(1);

    writeDword(p_comm, CRG_RESET_CONTROL, 0x3FFF);
}

/******************************************************************************
 *
 * ARTM_remove_forced_bluescreen()

 *
 ******************************************************************************/
void
ARTM_remove_forced_bluescreen(const CX_COMMUNICATION *p_comm, int ch_num)
{
	writeDword(p_comm, RE0_TRS_LOCK_THRESHOLD + (0x1000*ch_num), 0xf);
	WriteBitField(p_comm, RE0_DIAG_RE_CTRL0 + (0x1000*ch_num), 5, 4, 0);
}

/******************************************************************************
 *
 * ARTM_disableEnableRE()

 *
 ******************************************************************************/
void
ARTM_disableEnableRE(const CX_COMMUNICATION *p_comm, int ch_num)
{
    //unsigned long value = 0;
    //int result = TRUE;

    if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP) {
        writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F00);

        p_comm->sleep(1);

        writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F01);
    }
    else {
        if(ARTM_BLUE_FLD) {
            writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F00);

            p_comm->sleep(1);

            writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F01);
        }
        else {
            writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x1F00);

            p_comm->sleep(1);

            writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x1F01);
        }
    }

    //result &= readDword(p_comm, RE0_CSR + (0x1000 * ch_num), &value);
    //printk("ARTM_disableEnableRE:  ch_num = %d, value = 0x%x\n", ch_num, value);
}

/******************************************************************************
 *
 * ARTM_force_bluescreen()

 *
 ******************************************************************************/
void
ARTM_force_bluescreen(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned long value = 0;
    int result = TRUE;

    if (rate == ARTM_RATE_SD)
    {
        writeDword(p_comm, RE0_SRF_0 +(0x1000*ch_num), 0x18);    //setup SRF 5:0 = 0x24

    	writeDword(p_comm, RE0_SRF_1 +(0x1000*ch_num), 0x35A);   //setup SRF

    	writeDword(p_comm, RE0_SRF_2 +(0x1000*ch_num), 0x2D0);   //setup SRF
    	writeDword(p_comm, RE0_SRF_3 +(0x1000*ch_num), 0x106);   //setup SRF
    	writeDword(p_comm, RE0_SRF_4 +(0x1000*ch_num), 0x1);     //setup SRF
    	writeDword(p_comm, RE0_SRF_5 +(0x1000*ch_num), 0xF1);    //setup SRF
    	writeDword(p_comm, RE0_SRF_6 +(0x1000*ch_num), 0x15);    //setup SRF
    	writeDword(p_comm, RE0_SRF_7 +(0x1000*ch_num), 0x107);   //setup SRF
    	writeDword(p_comm, RE0_SRF_8 +(0x1000*ch_num), 0xF2);    //setup SRF
    	writeDword(p_comm, RE0_SRF_9 +(0x1000*ch_num), 0x2);     //setup SRF
    }
    else
    {
        writeDword(p_comm, RE0_SRF_0 + (0x1000*ch_num), 0x24);    //setup SRF 5:0 = 0x24

        writeDword(p_comm, RE0_SRF_1 + (0x1000*ch_num), 0x898);   //setup SRF

        writeDword(p_comm, RE0_SRF_2 + (0x1000*ch_num), 0x780);  //setup SRF
        writeDword(p_comm, RE0_SRF_3 + (0x1000*ch_num), 0x465);  //setup SRF
        writeDword(p_comm, RE0_SRF_4 + (0x1000*ch_num), 0x1);    //setup SRF
        writeDword(p_comm, RE0_SRF_5 + (0x1000*ch_num), 0x438);  //setup SRF
        writeDword(p_comm, RE0_SRF_6 + (0x1000*ch_num), 0x2A);   //setup SRF
        writeDword(p_comm, RE0_SRF_10+ (0x1000*ch_num), 0x7);    //setup SRF
    }

    //step 7
#ifdef ARTM_LOS_BLACK_COLOR
    writeDword(p_comm, RE0_FILL_Y1+(0x1000*ch_num), 0x0040);   //program BLACK COLOR
    writeDword(p_comm, RE0_FILL_Y2+(0x1000*ch_num), 0x0040);   //program BLACK COLOR
    writeDword(p_comm, RE0_FILL_CB+(0x1000*ch_num), 0x0200);   //program BLACK COLOR
    writeDword(p_comm, RE0_FILL_CR+(0x1000*ch_num), 0x0200);   //program BLACK COLOR
#else
    writeDword(p_comm, RE0_FILL_Y1 +(0x1000*ch_num), 0x1F6);   //program BLUE COLOR
    writeDword(p_comm, RE0_FILL_Y2 +(0x1000*ch_num), 0x1F6);   //program BLUE COLOR
    writeDword(p_comm, RE0_FILL_CB +(0x1000*ch_num), 0x3C0);   //program BLUE COLOR
    writeDword(p_comm, RE0_FILL_CR +(0x1000*ch_num), 0x40);    //program BLUE COLOR
#endif

    result &= readDword(p_comm, RE0_DIAG_RE_CTRL0 +(0x1000*ch_num), &value);

    if(ARTM_BLUE_FLD)
        writeDword(p_comm, RE0_DIAG_RE_CTRL0 +(0x1000*ch_num), value|0x0010);   //5:4 = 1

    ARTM_disableEnableRE ( p_comm, ch_num );

    //printk("ARTM_force_bluescreen: Done\n");
}

/******************************************************************************
 *
 * ARTM_enableBT1120()

 *
 ******************************************************************************/
void
ARTM_enableBT1120(const CX_COMMUNICATION *p_comm)
{
    writeDword(p_comm, VOF_OUTPUT_FORMAT_0, 0x00B4);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_1, 0x00B4);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_2, 0x00B4);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_3, 0x00B4);

    ARTM_resetClock( p_comm );

    ARTM_resetRECh ( p_comm, 0 );
    ARTM_resetRECh ( p_comm, 1 );
    ARTM_resetRECh ( p_comm, 2 );
    ARTM_resetRECh ( p_comm, 3 );

#ifdef ARTM_LOS_BLACK_COLOR
    {
        int i;

        /* Set Output Pattern as Black Color when LOS, the default is blue color when LOS,  Jerson */
        for(i=0; i<4; i++) {
            writeDword(p_comm, RE0_FILL_Y1+(0x1000*i), 0x0040);
            writeDword(p_comm, RE0_FILL_Y2+(0x1000*i), 0x0040);
            writeDword(p_comm, RE0_FILL_CB+(0x1000*i), 0x0200);
            writeDword(p_comm, RE0_FILL_CR+(0x1000*i), 0x0200);
        }
    }
#endif

    ARTM_disableEnableRE ( p_comm, 0 );
    ARTM_disableEnableRE ( p_comm, 1 );
    ARTM_disableEnableRE ( p_comm, 2 );
    ARTM_disableEnableRE ( p_comm, 3 );

    //printk("ARTM_enableBT1120: Done\n");
}

/******************************************************************************
 *
 * ARTM_enableBT656()

 *
 ******************************************************************************/
void
ARTM_enableBT656(const CX_COMMUNICATION *p_comm)
{
    writeDword(p_comm, VOF_OUTPUT_FORMAT_0, 0x00B5);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_1, 0x00B5);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_2, 0x00B5);
    writeDword(p_comm, VOF_OUTPUT_FORMAT_3, 0x00B5);

    ARTM_resetClock( p_comm );

    ARTM_resetRECh ( p_comm, 0 );
    ARTM_resetRECh ( p_comm, 1 );
    ARTM_resetRECh ( p_comm, 2 );
    ARTM_resetRECh ( p_comm, 3 );

#ifdef ARTM_LOS_BLACK_COLOR
    {
        int i;

        /* Set Output Pattern as Black Color when LOS, the default is blue color when LOS,  Jerson */
        for(i=0; i<4; i++) {
            writeDword(p_comm, RE0_FILL_Y1+(0x1000*i), 0x0040);
            writeDword(p_comm, RE0_FILL_Y2+(0x1000*i), 0x0040);
            writeDword(p_comm, RE0_FILL_CB+(0x1000*i), 0x0200);
            writeDword(p_comm, RE0_FILL_CR+(0x1000*i), 0x0200);
        }
    }
#endif

    ARTM_disableEnableRE ( p_comm, 0 );
    ARTM_disableEnableRE ( p_comm, 1 );
    ARTM_disableEnableRE ( p_comm, 2 );
    ARTM_disableEnableRE ( p_comm, 3 );

    //printk("ARTM_enableBT656: Done\n");
}

/******************************************************************************
 *
 * ARTM_setCableDistance()
 *
 ******************************************************************************/
void
ARTM_setCableDistance(const CX_COMMUNICATION *p_comm, int ch_num, int length)
{
    if (  length > 1 ) // 125 to 150m
    {
        ARTM_load_RG59_125to150m(p_comm, ch_num);
    }
    else if ( length > 0 ) // 75 to 100m
    {
        ARTM_load_RG59_75to100m(p_comm, ch_num);
    }
    else // 0 to 50m
    {
        ARTM_load_RG59_0to50m_auto(p_comm, ch_num);
    }

    //printk("ARTM_setCableDistance: Done\n");
}

/******************************************************************************
 *
 * ARTM_writeRegister()
 *
 ******************************************************************************/
int
ARTM_writeRegister(const CX_COMMUNICATION *p_comm,
                const unsigned long registerAddress,
                const unsigned long value)
{
    int result = TRUE;

    result &= writeDword(p_comm, registerAddress, value );

    return result;
}

/******************************************************************************
 *
 * ARTM_readRegister()
 *
 ******************************************************************************/
int
ARTM_readRegister(const CX_COMMUNICATION *p_comm,
               const unsigned long registerAddress,
               unsigned long *p_value)
{
    int result = TRUE;

    result &= readDword( p_comm, registerAddress, p_value );

    return result;
}

/******************************************************************************
* function    : ARTM_PowerDownTxChannel
* Description : power down tx channel
******************************************************************************/
int
ARTM_PowerDownTxChannel(const CX_COMMUNICATION *p_comm, int ch_num)
{
    int ret_val = 0;
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1 + ch_num*0x10, 14, 14, 1); // Power Down TX channel
    return ret_val;
}

/******************************************************************************
* function    : ARTM_PrintInputDataLevel
* Description : print input data level for all channels
******************************************************************************/
void
ARTM_PrintInputDataLevel(const CX_COMMUNICATION *p_comm)
{
    int artemis44_ch0_data_level = 0;
    int artemis44_ch1_data_level = 0;
    int artemis44_ch2_data_level = 0;
    int artemis44_ch3_data_level = 0;

    ReadBitField(p_comm, AMS_CH0_RXREG18, 7,4, (unsigned long *)&artemis44_ch0_data_level);
    ReadBitField(p_comm, AMS_CH1_RXREG18, 7,4, (unsigned long *)&artemis44_ch1_data_level);
    ReadBitField(p_comm, AMS_CH2_RXREG18, 7,4, (unsigned long *)&artemis44_ch2_data_level);
    ReadBitField(p_comm, AMS_CH3_RXREG18, 7,4, (unsigned long *)&artemis44_ch3_data_level);

    printk("Artemis Ch0/1/2/3 data level = %d %d %d %d\n", artemis44_ch0_data_level, artemis44_ch1_data_level, artemis44_ch2_data_level, artemis44_ch3_data_level);
}

/******************************************************************************
* function    : ARTM_EQLoopback
* Description : enable EQ loopback
******************************************************************************/
int
ARTM_EQLoopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num)
{
    int ret_val = 0;
    ret_val &= WriteBitField(p_comm, AMS_CH0_RXREG23 + (in_ch_num*0x80), 7, 4, 0x8);       // High Speed Mux Output = EQ
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG3 + (out_ch_num*0x10), 1, 0, in_ch_num); // Serial Loopback Source and Output selection
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1 + (out_ch_num*0x10), 14, 14, 0);       // Power up TX Ch
    return ret_val;
}

/******************************************************************************
* function    : ARTM_BT1120Loopback
* Description : enable BT1120 loopback
******************************************************************************/
int
ARTM_BT1120Loopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num, int data_rate)
{
    int ret_val = 0;
    int CDR_LOCK;
    int RE_CSR;

    ret_val &= WriteBitField(p_comm, AMS_CH0_RXREG23+(0x80*in_ch_num), 7, 4, 0);         // CDR output = Recovered Serial Clock

    ReadBitField(p_comm, AMS_CH0_RXREG19+(0x80*in_ch_num), 2, 2, (unsigned long *)&CDR_LOCK);

    if(!CDR_LOCK) {
        printk("CDR needs to be locked in order for BT1120 loopback to work\n\n");
    }

    ReadBitField(p_comm, RE0_CSR+(0x1000*in_ch_num), 31, 0, (unsigned long *)&RE_CSR);
    if(RE_CSR != 0x2F4B) {
        printk("ch%d RE_CSR = 0x%x\n\n" , in_ch_num, RE_CSR);
    }

    // Video Serial Transmit
    ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*out_ch_num), 9, 8, data_rate);
    ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*out_ch_num), 5, 4, 0x0);          //00 = Associated parallel video interface, 01 = Alternate parallel video interface, 10 = Associated video raster engine module
    ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*out_ch_num), 0, 0, 1);            // Enable

    if(in_ch_num != out_ch_num) {
        ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*out_ch_num), 0, 0, 0);        // Disable
        ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*out_ch_num), 5, 4, 0x1);      //00 = Associated parallel video interface, 01 = Alternate parallel video interface, 10 = Associated video raster engine module

        //This is just to allow loopback at the BT1120 pads
        ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*in_ch_num), 9, 8, data_rate); //
        ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*in_ch_num), 5, 4, 0x0);       //00 = Associated parallel video interface
        ret_val &= WriteBitField(p_comm, GC_VSTI0_CSR+(0x2*in_ch_num), 0, 0, 1);         // Enable
    }

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1+(0x10*out_ch_num), 14, 14, 0);       // Power up TX
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG3+(0x10*out_ch_num), 3, 3, 0);         // Power up Serializer for normal operation
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG3+(0x10*out_ch_num), 2, 2, 0);         // Use data from BT.1120 instead of DRV_SERDATA_SELC
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG3+(0x10*out_ch_num), 1, 0, in_ch_num); // Serial data/clock selection control for serial loopback operation. Encoded as: 0x0 = CH0_HSIN, 0x1 = CH1_HSIN, 0x2 = CH2_HSIN, 0x3 = CH3_HSIN

    ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_A_B, 15, 0,  0x0000);             //PATH TIMING ADJUSTMENT
    ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_C_D, 15, 0,  0x0000);

    if(data_rate == ARTM_RATE_HD || data_rate == ARTM_RATE_GEN3) {
        if(ARTM_get_Chip_RevID(p_comm) == ARTM_CHIP_REV_11ZP)
            ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_A_B, 15, 0,  0x0100);
        else
            ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_A_B, 15, 0,  0x0120);         //PATH TIMING ADJUSTMENT
        ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_C_D, 15, 0,  0x0001);
    }
    else if(data_rate == ARTM_RATE_SD) {
        ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_A_B, 15, 0,  0x0000);         //PATH TIMING ADJUSTMENT
        ret_val &= WriteBitField(p_comm, GC_VID_OUT_CONFIG_C_D, 15, 0,  0x0000);
    }

    return ret_val;
}

/******************************************************************************
* function    : ARTM_XREnable
* Description : enable Preemphasis for XR
******************************************************************************/
int
ARTM_XREnable(const CX_COMMUNICATION *p_comm, int ch_num, int dB)
{
    int ret_val = 0;
    int DRV_CUR_AMPCTLC = 14;
    int DRV_POST1CUR_AMPCTLC = 6;
    int DRV_POST1CUR_ENC = 1;
    int DRV_POST2CUR_AMPCTLC = 0;
    int DRV_POST2CUR_ENC = 0;
    int DRV_PRECUR_AMPCTLC = 0;
    int DRV_PRECUR_ENC = 0;

    if(dB == 6){
        DRV_CUR_AMPCTLC = 24;
        DRV_POST1CUR_AMPCTLC = 15;
    }

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 6, 6, 1);   // Set POST1CUR_SGNC = 1
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 14, 14, 1); // Set POST2CUR_SGNC = 1

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1+(0x10*ch_num), 12, 8, DRV_CUR_AMPCTLC);

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 3, 0, DRV_POST1CUR_AMPCTLC);
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 7, 7, DRV_POST1CUR_ENC);

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 10, 8, DRV_POST2CUR_AMPCTLC);
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG2+(0x10*ch_num), 15, 15, DRV_POST2CUR_ENC);

    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1+(0x10*ch_num), 2, 0, DRV_PRECUR_AMPCTLC);
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1+(0x10*ch_num), 7, 7, DRV_PRECUR_ENC);

    return ret_val;
}

/******************************************************************************
* function    : ARTM_CDRLoopback
* Description : enable CDR loopback
******************************************************************************/
int
ARTM_CDRLoopback(const CX_COMMUNICATION *p_comm, int in_ch_num, int out_ch_num)
{
    int ret_val = 0;
    ret_val &= WriteBitField(p_comm, AMS_CH0_RXREG23 + (in_ch_num*0x80), 7, 4, 0x4);        // High Speed Mux Output = CDR
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG3 + (out_ch_num*0x10), 1, 0, in_ch_num);  // Serial Loopback Source and Output selection
    ret_val &= WriteBitField(p_comm, AMS_CH0_TXREG1 + (out_ch_num*0x10), 14, 14, 0);        // Power up TX Ch
    return ret_val;
}

/******************************************************************************
* function    : ARTM_getVideoFormat
* Description : get video input source format
******************************************************************************/
int
ARTM_getVideoFormat(const CX_COMMUNICATION *p_comm, int ch_num, int* p_format)
{
    unsigned long val;
    unsigned long FormatIndex, M_Factor, PixelRate, IorP, NumPixels;
    int ret_val = TRUE;

    *p_format = 0;

    ret_val &= readDword( p_comm, RE0_DRF_0 + (ch_num*0x1000), &val );
    ret_val &= readDword( p_comm, RE0_DRF_1 + (ch_num*0x1000), &NumPixels );

    if((val & 0x1) == 0)    ///< format not detected
        return ret_val;

    FormatIndex = (val & 0x1F00) >> 8;
    M_Factor	= (val & 0x00C0) >> 6;
    PixelRate	= (val & 0x0030) >> 4;
    IorP		= (val & 0x0008) >> 3;
    NumPixels	= NumPixels & 0x1FFF;

    //if(0){
    //    printk("\n\tFormatIndex = 0x%x", FormatIndex);
    //    printk("\n\tM_Factor = 0x%x", M_Factor);
    //    printk("\n\tPixelRate = 0x%x", PixelRate);
    //    printk("\n\tIorP = 0x%x", IorP);
    //    printk("\n\tNumPixels = 0x%x\n\n", NumPixels);
    //}

    if( FormatIndex > 0 )
    {
        *p_format = FormatIndex;
        return ret_val;
    }

    switch ( NumPixels )
    {
    	case 858: 	FormatIndex = 0x1; break;
    	case 864: 	FormatIndex = 0x2; break;
    	case 1650: 	FormatIndex = ( (M_Factor == 0x1) ? 0x3 : 0xD); break;
    	case 1980:	FormatIndex = 0x4; break;
    	case 2200:
    		if( IorP )
    			FormatIndex = ( (M_Factor == 0x1) ? 0x8 : 0x10);
    		else if( PixelRate == 0x2 )
    			FormatIndex = ( (M_Factor == 0x1) ? 0xA : 0x11);
    		else
    			FormatIndex = ( (M_Factor == 0x1) ? 0x13 : 0x15);
    		break;
    	case 2640:
    		if( IorP )
    			FormatIndex = 0x9;
    		else if( PixelRate == 0x2 )
    			FormatIndex = 0xB;
    		else
    			FormatIndex = 0x14;
    		break;
    	case 2750: 	FormatIndex = ( (M_Factor == 0x1) ? 0xC : 0x12); break;
    	case 3300: 	FormatIndex = ( (M_Factor == 0x1) ? 0x5 : 0xE); break;
    	case 3960: 	FormatIndex = 0x6; break;
    	case 4125: 	FormatIndex = ( (M_Factor == 0x1) ? 0x7 : 0xF); break;
    }

    //printk("\n\tFormatIndex = 0x%x\n\n", FormatIndex);

    *p_format = FormatIndex;
    return ret_val;
}

/******************************************************************************
 *
 * ARTM_checkCablePlugin()

 *
 ******************************************************************************/
int
ARTM_isCablePlugin(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value   = 0;
    int result = TRUE;

    result &= readDword(p_comm, AMS_CH0_RXREG18 + (0x80*ch_num), &value);

    if ( (value & 0x0001) == 0x0 )
    {
        //printf("ch%d LOS = 0\n\n", ch_num);
        return 1;
    }
    else
    {
        //printf("ch%d LOS = 1 \n\n", ch_num);
        return 0;
    }
}

/******************************************************************************
* function    : ARTM_Enable_GPIOC_MCLK
* Description : Set the I2S_2 MCLK output.
******************************************************************************/
int
ARTM_Enable_GPIOC_MCLK(const CX_COMMUNICATION *p_comm)
{
    int ret_val = 0;

    ret_val &= WriteBitField(p_comm, AMS_AUDPLL_CONTROL_HI, 3, 3, 0); // AUDPLL_PDC = 0 power up audio PLL
    ret_val &= WriteBitField(p_comm, AMS_AUDPLL_CONTROL_HI, 1, 1, 0); // AUDPLL_BYPASSC = 0 normal PLL output

    ret_val &= WriteBitField(p_comm, CRG_CLOCK_CONTROL, 12, 12, 1);   // Enable clocks to Audio SRC and I2S modules
    ret_val &= WriteBitField(p_comm, CRG_RESET_CONTROL, 13, 13, 1);   // De-assert I2S reset

    ret_val &= WriteBitField(p_comm, GC_PIN_REMAP_I2S, 11, 11, 1); // Enable I2C2_MCLK pin
    ret_val &= WriteBitField(p_comm, I2S_CSR_B, 10, 10, 1); // Enable MCLK_EN

    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 0, 0, 1); // Enable I2S 0 so MCLK can be generated

    return ret_val;
}

/* ========================================================================================================== */

/******************************************************************************
* function    : ARTM_AudioInit
* Description : initialize.
******************************************************************************/
int
ARTM_AudioInit(const CX_COMMUNICATION *p_comm, CX25930_AUDIO_EXTRACTION *aud_extract)
{
    int ret_val = TRUE;
    int ch_num;

    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 3, 0, 0); // Disable I2S first

    for(ch_num=0; ch_num<4; ch_num++)
    {
        ret_val &= WriteBitField(p_comm, ADE0_STOP_THRESHOLDS +(0x1000*ch_num), 3, 0, 0x2); // A0 Fix to update Available channel correctly

        ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 6, 4, aud_extract->extraction_mode);
        ret_val &= WriteBitField(p_comm, ADE0_SPEC_CHAN +(0x1000*ch_num), 3, 0, aud_extract->channel);
        ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 1, 1, 1); // enable_extraction = 1
        ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 0, 0, 1); // enable_module = 1

        //ret_val &= WriteBitField(p_comm, I2S_CHAN_MAP_0_3, 4*(ch_num+1), 4*ch_num, (1<<3)|(ch_num<<1)|0);
        ret_val &= WriteBitField(p_comm, I2S_CHAN_MAP_0_3, 16, 0, 0xec8a);
    }

    ret_val &= WriteBitField(p_comm, I2S_CSR_B, 3, 2, aud_extract->extended_ws_format);
    ret_val &= WriteBitField(p_comm, I2S_CSR_B, 1, 0, aud_extract->extended_mode);

    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 9, 8, aud_extract->slot_width);
    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 7, 7, aud_extract->master_mode);
    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 6, 4, aud_extract->sample_rate);
    ret_val &= WriteBitField(p_comm, I2S_CSR_A, 3, 0, aud_extract->enable_i2s);

    //printk("Audio Init Done\n");

    return ret_val;
}

/******************************************************************************
* function    : ARTM_AudioInitCh
* Description : initialize.
******************************************************************************/
int
ARTM_AudioInitCh(const CX_COMMUNICATION *p_comm, int ch_num)
{
    int ret_val = TRUE;

    ret_val &= WriteBitField(p_comm, ADE0_STOP_THRESHOLDS +(0x1000*ch_num), 3, 0, 0x2); // A0 Fix to update Available channel correctly

    ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 6, 4, 0);
    ret_val &= WriteBitField(p_comm, ADE0_SPEC_CHAN +(0x1000*ch_num), 3, 0, 0);
    ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 1, 1, 1); // enable_extraction = 1
    ret_val &= WriteBitField(p_comm, ADE0_CSR +(0x1000*ch_num), 0, 0, 1); // enable_module = 1

    return ret_val;
}

/******************************************************************************
* function    : ARTM_Pin_Remap_I2S
* Description : Set 1 to map pins as I2S, 0 as GPIOs.
******************************************************************************/
int
ARTM_PinRemap_I2S(const CX_COMMUNICATION *p_comm, int ch_num, unsigned char MCLK, unsigned char WS, unsigned char SCK, unsigned char I2S_SD)
{
    int ret_val = TRUE;

    ret_val &= WriteBitField(p_comm, GC_PIN_REMAP_I2S, 3+(ch_num*4), 3+(ch_num*4), MCLK);
    ret_val &= WriteBitField(p_comm, GC_PIN_REMAP_I2S, 2+(ch_num*4), 2+(ch_num*4), WS);
    ret_val &= WriteBitField(p_comm, GC_PIN_REMAP_I2S, 1+(ch_num*4), 1+(ch_num*4), SCK);
    ret_val &= WriteBitField(p_comm, GC_PIN_REMAP_I2S, 0+(ch_num*4), 0+(ch_num*4), I2S_SD);

    return ret_val;
}

/******************************************************************************
 *
 * ARTM_BurstTransfer() ///< Jerson
 *
 ******************************************************************************/
int
ARTM_BurstTransfer_Ctrl(const CX_COMMUNICATION *p_comm, int enable, unsigned char inc_amount, unsigned short burst_len)
{
    unsigned long value;
    int result = TRUE;

    if(enable && ((inc_amount > 4) || ((burst_len%2) != 0)))
        return FALSE;

    result &= readDword(p_comm, CSI_CONTROL_00, &value);
    if(result) {
        value &= ~0x61;
        if(enable) {
            /* ADR_INC and BLA_ENA */
            value |= ((inc_amount<<5) | 0x1);
            result &= writeDword(p_comm, CSI_CONTROL_00, value);

            /* Burst Length */
            result &= writeDword(p_comm, CSI_CONTROL_20, (burst_len & 0xff));       ///< LSB
            result &= writeDword(p_comm, CSI_CONTROL_24, ((burst_len>>8)&0xff));    ///< MSB
        }
        else {
            result &= writeDword(p_comm, CSI_CONTROL_00, value);
            result &= writeDword(p_comm, CSI_CONTROL_20, 0);
            result &= writeDword(p_comm, CSI_CONTROL_24, 0);
        }
    }

    return result;
}

/******************************************************************************
 *
 * ARTM_resetRE()

 *
 ******************************************************************************/
void
ARTM_resetRE(const CX_COMMUNICATION *p_comm, int ch_num)
{
    writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F00);

    p_comm->sleep(1);

    writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F01);
}

/******************************************************************************
 *
 * ARTM_LoopInit()
 *
 ******************************************************************************/
void
ARTM_LoopInit_11ZP(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned long FN_GAIN = 0;
    unsigned long DC_GAIN = 0;

    WriteBitField(p_comm, AMS_CH0_RXREG5  + (ch_num*0x80), 15, 0, 0x0010); // EQ_ANACTLC = 0x0010

    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);       // auto FN gain
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);       // auto DC gain

    WriteBitField(p_comm, AMS_CH0_RXREG5  + (ch_num*0x80), 15, 0, 0x0010); // EQ_ANACTLC = 0x0010
    WriteBitField(p_comm, AMS_CH0_RXREG8  + (ch_num*0x80), 6, 6, 0x0);     // EQ__DIG_AUTO_RATE_ENA = 0
    if(rate == ARTM_RATE_SD){
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x1);  // EQ__DIG_DATA_RATE = 1
    }else if(rate == ARTM_RATE_HD){
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x2);  // EQ__DIG_DATA_RATE = 2
    }else if(rate == ARTM_RATE_GEN3){
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x3);  // EQ__DIG_DATA_RATE = 3
    }

    WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 6, 6, 0x1);      // EQ__DIG_AUTO_RATE_ENA = 1
    WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 6, 6, 0x0);      // EQ__DIG_AUTO_RATE_ENA = 0

    p_comm->sleep(100);
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);       // EQ_DIG_DC_LOOP_MAN = 1
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, 0x8F);    // EQ_DIG_DC_GAIN = 0x8F
    WriteBitField(p_comm, AMS_CH0_RXREG7 + (ch_num*0x80), 2, 2, 1);        // RESET
    WriteBitField(p_comm, AMS_CH0_RXREG7 + (ch_num*0x80), 2, 2, 0);        // Out of reset
    p_comm->sleep(100);
    ReadBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, &FN_GAIN); // Read EQ_DIG_FN_GAIN
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);       // EQ_DIG_FN_LOOP_MAN = 1
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN); // EQ_DIG_FN_GAIN = CHx_FN_GAIN

    ReadBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, &DC_GAIN); // Read EQ_DIG_DC_GAIN
}

void
ARTM_Loop_11ZP(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long DC_GAIN = 0;
    unsigned long FN_GAIN = 0;
    unsigned long Rate    = 0;

    unsigned long hd_dc_thresh   = 0x44;
    unsigned long hd_fn_thresh   = 0x55;
    unsigned long sd_dc_thresh   = 0x44;
    unsigned long sd_fn_thresh   = 0x4d;
    unsigned long gen3_dc_thresh = 0x3a;
    unsigned long gen3_fn_thresh = 0x62;

    ReadBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 3, 2, &Rate);                   // Read Rate
    if(Rate == ARTM_RATE_HD) {
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 15, 9, hd_dc_thresh);     // UPPER_TARGET_IN_HD = chx_hd_dc_thresh
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 8, 2, hd_dc_thresh);      // LOWER_TARGET_IN_HD = chx_hd_dc_thresh
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);                 // EQ_DIG_DC_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm,  AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, &DC_GAIN);          // Read EQ_DIG_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);                 // EQ_DIG_DC_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, DC_GAIN);           // EQ_DIG_DC_GAIN = CHx_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 15, 9, hd_fn_thresh);     // UPPER_TARGET_IN_HD = chx_hd_fn_thresh
        WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 8, 2, hd_fn_thresh);      // LOWER_TARGET_IN_HD = chx_hd_fn_thresh

        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);                 // FN_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm,  AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, &FN_GAIN);          // Read EQ_DIG_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);                 // FN_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN);           // FN_GAIN = CHx_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG5 + (ch_num*0x80), 15, 0, 0x0000);            // EQ_ANACTLC = 0x0000
        //printk("Ran loop HD ch%d\n", ch_num);
    }
    else if(Rate == ARTM_RATE_SD) {
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 15, 9, sd_dc_thresh);     // UPPER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 8, 2, sd_dc_thresh);      // LOWER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);                 // EQ_DIG_DC_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, &DC_GAIN);           // Read EQ_DIG_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);                 // EQ_DIG_DC_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, DC_GAIN);           // EQ_DIG_DC_GAIN = CHx_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 15, 9, sd_fn_thresh);     // UPPER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 8, 2, sd_fn_thresh);      // LOWER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);                 // FN_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, &FN_GAIN);           // Read EQ_DIG_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);                 // FN_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN);           // FN_GAIN = CHx_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG5 + (ch_num*0x80), 15, 0, 0x0000);            // EQ_ANACTLC = 0x0000
        //printk("Ran loop SD ch%d\n" ,ch_num);
    }
    else if(Rate == ARTM_RATE_GEN3) {
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 15, 9, gen3_dc_thresh);    // UPPER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 8, 2, gen3_dc_thresh);     // LOWER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);                 // EQ_DIG_DC_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, &DC_GAIN);           // Read EQ_DIG_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);                 // EQ_DIG_DC_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, DC_GAIN);           // EQ_DIG_DC_GAIN = CHx_DC_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 15, 9, gen3_fn_thresh);    // UPPER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG9 + (ch_num*0x80), 8, 2, gen3_fn_thresh);     // LOWER_TARGET_IN_
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);                 // FN_LOOP_MAN = 0
        p_comm->sleep(100);
        ReadBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, &FN_GAIN);           // Read EQ_DIG_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);                 // FN_LOOP_MAN = 1
        WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN);           // FN_GAIN = CHx_FN_GAIN
        WriteBitField(p_comm, AMS_CH0_RXREG5 + (ch_num*0x80), 15, 0, 0x0000);            // EQ_ANACTLC = 0x0000
        //printk("Ran loop 3G ch%d\n", ch_num);
    }
    else {
        //printk("Rate mistmatch\n");
    }
}

int ARTM_DcGainLUT(const CX_COMMUNICATION *p_comm, int ch_num)
{
	unsigned long value;
	int dc_gain=0;

	ReadBitField(p_comm, AMS_CH0_RXREG18 + (0x80*ch_num), 7, 4, &value);

	switch( value ) {
	    case  0:
	    case  1:
	    case  2:
	    case  3: dc_gain=0xff; break;
	    case  4: dc_gain=0xf0; break;
	    case  5: dc_gain=0xe0; break;
	    case  6: dc_gain=0xd0; break;
	    case  7: dc_gain=0xc0; break;
	    case  8: dc_gain=0xb0; break;
	    case  9: dc_gain=0xa0; break;
	    case 10: dc_gain=0x90; break;
	    case 11: dc_gain=0x80; break;
	    case 12: dc_gain=0x70; break;
	    case 13: dc_gain=0x6f; break;
	    case 14: dc_gain=0x6e; break;
	    case 15: dc_gain=0x6d; break;
	}

	return dc_gain;
}

int ARTM_FnGainLUT(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long value;
    int fn_gain=0;

    ReadBitField(p_comm, AMS_CH0_RXREG18 + (0x80*ch_num), 7, 4, &value);

    switch( value ) {
        case  0:
        case  1:
        case  2:
        case  3: fn_gain=0x68; break;
        case  4: fn_gain=0x64; break;
        case  5: fn_gain=0x60; break;
        case  6: fn_gain=0x5c; break;
        case  7: fn_gain=0x58; break;
        case  8: fn_gain=0x50; break;
        case  9: fn_gain=0x48; break;
        case 10: fn_gain=0x40; break;
        case 11: fn_gain=0x3c; break;
        case 12: fn_gain=0x38; break;
        case 13: fn_gain=0x30; break;
        case 14: fn_gain=0x28; break;
        case 15: fn_gain=0x20; break;
    }

    return fn_gain;
}

void ARTM_LoopInit(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned long FN_GAIN = 0;
    unsigned long DC_GAIN = 0;
    unsigned long value   = 0;
    unsigned long hd_dc_thresh   = 0x5A;
    unsigned long hd_fn_thresh   = 0x56;
    unsigned long sd_dc_thresh   = 0x44;
    unsigned long sd_fn_thresh   = 0x4d;
    unsigned long gen3_dc_thresh = 0x3a;
    unsigned long gen3_fn_thresh = 0x62;

    WriteBitField(p_comm, AMS_CH0_RXREG5 + (ch_num*0x80), 15, 0, 0x0010); // EQ_ANACTLC = 0x0010

    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);      // auto FN gain
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);      // auto DC gain

    WriteBitField(p_comm, AMS_CH0_RXREG5 + (ch_num*0x80), 15, 0, 0x0010); // EQ_ANACTLC = 0x0010
    WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 6, 6, 0x0);     // EQ__DIG_AUTO_RATE_ENA = 0

    readDword(p_comm, AMS_CH0_RXREG23 +(0x80*ch_num), &value);

    if(rate == ARTM_RATE_SD) {
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 3, 2, 0x1);     // JG: For A1
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x1);     // EQ__DIG_DATA_RATE = 1
        writeDword(p_comm, AMS_CH0_RXREG23 + (ch_num*0x80), (value & ~0x0300) | (0x1 << 8) );   // CDR_FB_DIV_SELC = 0x1 3G Mode
    }
    else if(rate == ARTM_RATE_HD) {
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 3, 2, 0x2);     // JG: For A1
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x2);     // EQ__DIG_DATA_RATE = 2
        writeDword(p_comm, AMS_CH0_RXREG23+(ch_num*0x80), (value & ~0x0300) | (0x2 << 8) );   // CDR_FB_DIV_SELC = 0x2 HD Mode
    }
    else if(rate == ARTM_RATE_GEN3) {
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 3, 2, 0x3);     // JG: For A1
        WriteBitField(p_comm, AMS_CH0_RXREG8 + (ch_num*0x80), 5, 4, 0x3);     // EQ__DIG_DATA_RATE = 3
        writeDword(p_comm, AMS_CH0_RXREG23 + (ch_num*0x80), (value & ~0x0300) | (0x3 << 8) );   // CDR_FB_DIV_SELC = 0x3 3G Mode
    }

    p_comm->sleep(100);

    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);      // EQ_DIG_DC_LOOP_MAN = 1
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, 0x8F);   // EQ_DIG_DC_GAIN = 0x8F
    WriteBitField(p_comm, AMS_CH0_RXREG7  + (ch_num*0x80), 2, 2, 1);      // RESET
    WriteBitField(p_comm, AMS_CH0_RXREG7  + (ch_num*0x80), 2, 2, 0);      // Out of reset

    p_comm->sleep(100);

    ReadBitField(p_comm, AMS_CH0_RXREG12  + (ch_num*0x80), 7, 0, &FN_GAIN);          // Read EQ_DIG_FN_GAIN
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);                 // EQ_DIG_FN_LOOP_MAN = 1
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN);           // EQ_DIG_FN_GAIN = CHx_FN_GAIN

    WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 15, 9, hd_dc_thresh);     // UPPER_TARGET_IN_
    WriteBitField(p_comm, AMS_CH0_RXREG10 + (ch_num*0x80), 8, 2, hd_fn_thresh);      // LOWER_TARGET_IN_
    WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 15, 9, sd_dc_thresh);     // UPPER_TARGET_IN_
    WriteBitField(p_comm, AMS_CH0_RXREG11 + (ch_num*0x80), 8, 2, sd_fn_thresh);      // LOWER_TARGET_IN_
    WriteBitField(p_comm, AMS_CH0_RXREG9  + (ch_num*0x80), 15, 9, gen3_dc_thresh);   // UPPER_TARGET_IN_
    WriteBitField(p_comm, AMS_CH0_RXREG9  + (ch_num*0x80), 8, 2, gen3_fn_thresh);    // LOWER_TARGET_IN_

    ReadBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, &DC_GAIN);           // Read EQ_DIG_DC_GAIN
}

void ARTM_Loop(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long DC_GAIN = 0;
    unsigned long FN_GAIN = 0;
    unsigned long in_lvl  = 0;

    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 0);           // EQ_DIG_DC_LOOP_MAN = 0

    p_comm->sleep(100);

    ReadBitField(p_comm, AMS_CH0_RXREG18  + (0x80*ch_num), 7, 4, &in_lvl);     // Read input level
    ReadBitField(p_comm, AMS_CH0_RXREG13  + (ch_num*0x80), 7, 0, &DC_GAIN);    // Read EQ_DIG_DC_GAIN
    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 8, 8, 1);           // EQ_DIG_DC_LOOP_MAN = 1

    // DC gain railing protection
    if((in_lvl >= 0xD) || (DC_GAIN<0x30) || (DC_GAIN>0xF8))
        DC_GAIN = ARTM_DcGainLUT(p_comm, ch_num);

    WriteBitField(p_comm, AMS_CH0_RXREG13 + (ch_num*0x80), 7, 0, DC_GAIN);     // EQ_DIG_DC_GAIN = CHx_DC_GAIN
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 0);           // FN_LOOP_MAN = 0

    p_comm->sleep(100);

    ReadBitField(p_comm, AMS_CH0_RXREG18  + (0x80*ch_num), 7, 4, &in_lvl);     // Read input level
    ReadBitField(p_comm, AMS_CH0_RXREG12  + (ch_num*0x80), 7, 0, &FN_GAIN);    // Read EQ_DIG_FN_GAIN
    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 8, 8, 1);           // FN_LOOP_MAN = 1

    if((FN_GAIN<0x10) || (FN_GAIN>0x70))            // FN gain railing protection
        FN_GAIN = ARTM_FnGainLUT(p_comm, ch_num);

    WriteBitField(p_comm, AMS_CH0_RXREG12 + (ch_num*0x80), 7, 0, FN_GAIN);     // FN_GAIN = CHx_FN_GAIN
}

void ARTM_FractionalFrames(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long cdr_lockc = 0;
    int cdr_delay = 0;

    ReadBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 2, 2, &cdr_lockc);

    if(!cdr_lockc) {
        // This helps to reduce the wait time by only runnig this if CDR is not locked
        cdr_delay = 1;
        WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 1);    // Hold EQ CTLE in reset
        WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 1);  // CDR_FORCE_LOCK2REFC = 1
    }

    if(cdr_delay) {
        p_comm->sleep(300);
        if(!cdr_lockc) {
            WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 0); // CDR_FORCE_LOCK2REFC = 0
            WriteBitField(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), 2, 2, 0);    // Remove EQ CTLE from reset
        }
    }
}

/******************************************************************************
 *
 * ARTM_EQ_Check()
 *
 ******************************************************************************/
void ARTM_EQ_Check(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned int loopcount = 0;
    int chip_rev = ARTM_get_Chip_RevID(p_comm);

    if(chip_rev == ARTM_CHIP_REV_11ZP) {
        ARTM_LoopInit_11ZP(p_comm, ch_num, rate);
        while(!ARTM_ReadLOS(p_comm, ch_num)&&(loopcount < 8)) {
            ARTM_Loop_11ZP(p_comm, ch_num);
            loopcount++;
        }
    }
    else {
        ARTM_LoopInit(p_comm, ch_num, rate);
        while(!ARTM_ReadLOS(p_comm, ch_num) && (loopcount < 4)) {
            ARTM_Loop(p_comm, ch_num);
            loopcount++;
        }
        ARTM_FractionalFrames(p_comm, ch_num);
    }
}

/******************************************************************************
 *
 * ARTM_LockToFractionalFrames()
 *
 ******************************************************************************/
// This needs to be executed after the input is plugged in to get fractional frame rates to lock
void ARTM_LockToFractionalFrames(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long cdr_lockc = 0;

    ReadBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 2, 2, &cdr_lockc);
    if(!cdr_lockc) { // This helps to reduce the wait time by only runnig this if CDR is not locked
        WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 1);    // Hold EQ CTLE in reset
        WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 1); // CDR_FORCE_LOCK2REFC = 1
        p_comm->sleep(300);
        WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 0); // CDR_FORCE_LOCK2REFC = 0
        WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 0);    // Remove EQ CTLE from reset
    }
}

/******************************************************************************
 *
 * ARTM_resetCh()
 *
 ******************************************************************************/
void ARTM_resetCh(const CX_COMMUNICATION *p_comm, int ch_num, int rate)
{
    unsigned long value = 0;
    int result = TRUE;

    /* CDR soft reset */
    result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value & ~0x1000) );

    /* EQ Soft Reset */
    result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value | 0x1000) );

    result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
    writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value & ~0x1000) );

    p_comm->sleep(500);

    writeDword(p_comm, AMS_CH0_RXREG5  + (0x80*ch_num), 0x0000 );          // Disable HPF Filter

    ARTM_initEQ(p_comm, ch_num, rate);
    ARTM_resetRECh(p_comm, ch_num);
}

/******************************************************************************
 *
 * ARTM_checkRasterEngineStatus()
 *
 ******************************************************************************/
int ARTM_checkRasterEngineStatus(const CX_COMMUNICATION *p_comm, struct artm_re_info_t *info, int ch_num)
{
    unsigned long CDR_PH_DWNCTLC, CDR_PH_UPCTLC;
    unsigned long re_csr  = 0;
    unsigned long rxreg19 = 0;
    unsigned long cdr_vcocalib_endc = 0;
    unsigned long value = 0;
    int result    = TRUE;
    int lock      = 0;
    int cdr_lockc = 0;
    int cdr_delay = 0;
    int re_delay  = 0;

    /* Check Raster Enginer Status */
    readDword(p_comm, RE0_CSR + (0x1000 * ch_num), &re_csr);

    if((re_csr&0xff) == 0x4B)
        lock = 1;

    if(lock == 0) {
        re_delay = 1;
        result &= readDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), &value);
        writeDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), (value | 0x4) );     // set EQ soft reset

        result &= readDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), &value);
        writeDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), (value & ~0x4) );    // release EQ soft reset

        if(info->input_rate != ARTM_RATE_SD) {
            result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
            writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value | 0x1000) );     // set CDR reset
            result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
            writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value & ~0x1000) );    // release CDR reset
        }

        ReadBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 2, 2, &rxreg19);

        cdr_lockc = rxreg19;

        if(!rxreg19) {
            // RayC Check VCO calibration and force a value if it failed
            ReadBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 1, 1, &cdr_vcocalib_endc);

            if(!cdr_vcocalib_endc) {
                result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), &value);
                writeDword(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), (value & 0xFF87) | 0x0418 ); // Set bit[6:3]CDR_VCOCALIB_OUTC = 0x3 and bit[10] CDR_VCOCALIB_OVERRIDEC = 1

                result &= readDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), &value);
                CDR_PH_UPCTLC  = (value & 0xf800) >> 11;
                CDR_PH_DWNCTLC = (value & 0x07c0) >> 6;

                if((CDR_PH_UPCTLC == 0x1f) || (CDR_PH_DWNCTLC == 0x1f))
                    writeDword(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), (value & 0x003F) );     // Write CDR_PH_UPCTLC/DWNCTLC = 0x0 for now, or can do CDR calibration instead.

                result &= readDword(p_comm, AMS_CH0_RXREG20 + (0x80*ch_num), &value);

                ReadBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 2, 2, &rxreg19 );
            }
            // RayC End check VCO calibration

            // This helps to reduce the wait time by only runnig this if CDR is not locked
            cdr_delay = 1;
            cdr_lockc = rxreg19;
            WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 1);    // Hold EQ CTLE in reset
            WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 1);  // CDR_FORCE_LOCK2REFC = 1
        }
    }

    if(cdr_delay)
        p_comm->sleep(300);

    if(re_delay) {
        if(!cdr_lockc) {
            WriteBitField(p_comm, AMS_CH0_RXREG19 + (0x80*ch_num), 14, 14, 0);  // CDR_FORCE_LOCK2REFC = 0
            WriteBitField(p_comm, AMS_CH0_RXREG7  + (0x80*ch_num), 2, 2, 0);    // Remove EQ CTLE from reset
        }

        ARTM_disableEnableRE(p_comm, ch_num);

        p_comm->sleep(300);

        result &= readDword(p_comm, RE0_CSR + (0x1000 * ch_num), &re_csr);

        if((re_csr&0xff) == 0x4B)
            lock = 1;
        else {
            // Jerson Add for EQ re-scan
            ARTM_LoopInit(p_comm, ch_num, info->input_rate);
            ARTM_Loop(p_comm, ch_num);
            ARTM_FractionalFrames(p_comm, ch_num);
        }
    }

    return lock;
}

void
ARTM_checkRasterEngineStatus_11ZP(const CX_COMMUNICATION *p_comm, struct artm_re_info_t *info, int ch_num)
{
    unsigned long value   = 0;
    unsigned long value1  = 0;
    unsigned long re_csr  = 0;
    unsigned long rxreg19 = 0;
    int result = TRUE;

    result &= readDword(p_comm, RE0_CSR + (0x1000 * ch_num), &re_csr);

    if ( (re_csr&0xff) != 0x4B ) {
        /* Read SDI CDR Status */
        result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80 * ch_num), &rxreg19);

        if ( (rxreg19&0xff) != 0x46 )    /* Enter if SDI CDR is not locked */
        {
            result &= readDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), &value);
            writeDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), (value | 0x4) );
            p_comm->sleep(10);

            result &= readDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), &value);
            writeDword(p_comm, AMS_CH0_RXREG7 + (0x80*ch_num), (value & ~0x4) );

            p_comm->sleep(10);
            result &= readDword(p_comm, AMS_CH0_RXREG19 + (0x80 * ch_num), &rxreg19);
        }
        else { //AMS = 0x8846, RE not responding, enable RE then check if necessary to walk CDR_PH
            result &= readDword(p_comm, CRG_RESET_CONTROL, &value);
            WriteBitField(p_comm, CRG_RESET_CONTROL, ch_num, ch_num, 0);

            p_comm->sleep(10);
            WriteBitField(p_comm, CRG_RESET_CONTROL, ch_num, ch_num, 1);

            p_comm->sleep(10);
            WriteBitField(p_comm,RE0_CSR + (0x1000 * ch_num), 14, 14, 0);  //disable sync line switch enable
            WriteBitField(p_comm,RE0_CSR + (0x1000 * ch_num), 0, 0, 1);    //re-enable RE

            p_comm->sleep(10);
            ARTM_remove_forced_bluescreen(p_comm, ch_num);
            result &= readDword(p_comm, RE0_CSR + (0x1000 * ch_num), &re_csr);

            if((re_csr&0xff) != 0x4B) {   //if RE still doesn't respond, walk CDR_PH
                info->settled = FALSE;   //if RE not responde, reset settled variable

                if(info->cdr_ph_up){//walking up
                    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, ++info->value_up);
                    if(info->value_up == 0x1F){  //boundary reset, set back 0x0 and switch to waking down
                        info->value_up = 0x0;
                        WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, info->value_up);
                        info->cdr_ph_up = FALSE;
                    }
                }
                else {  //walking down
                    WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, ++info->value_down);
                    if(info->value_down == 0x1F){   //boundary reset, set back to 0x0 and switch to walking up
                        info->value_down = 0x0;
                        WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, info->value_down);
                        info->cdr_ph_up = TRUE;
                    }
                }
            }else if((re_csr&0xff) == 0x4B){ //If CDR change brings video out, use the better CDR value here by adding two

            }
        }
    }else { //csr = 0x2F4B
        ReadBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, &value);
        ReadBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, &value1);
        if(((value > 0)||(value1 > 0))&&(info->settled == FALSE)) {  //when was not settled but up/down value changed that caused RE responds
            if(info->cdr_ph_up){
                if((info->value_up+3) > 0x1F){//if adding 3 over the boundary, use ceiling value
                    info->value_up  = 0x1F;
                    info->cdr_ph_up = FALSE;
                }else{
                    info->value_up += 3;//Experimenets show best result at CDR start working value plus 2 or 3.
                }
                WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 15, 11, info->value_up);
                info->settled  = TRUE;
                info->value_up = 0x0;
            }
            else {
                if((info->value_down+3) > 0x1F){
                    info->value_down = 0x1F;
                    info->cdr_ph_up = TRUE;
                }else{
                    info->value_down += 3;
                }
                WriteBitField(p_comm, AMS_CH0_RXREG21 + (0x80*ch_num), 10, 6, info->value_down);
                info->settled    = TRUE;
                info->value_down = 0x0;
            }
        }
    }

    //printk("ARTM_checkRasterEngineStatus:  ch_num = %d, re_csr = 0x%x\n", ch_num, re_csr);
}

/******************************************************************************
 *
 * ARTM_get_Chip_RevID()
 *
 ******************************************************************************/
int ARTM_get_Chip_RevID(const CX_COMMUNICATION *p_comm)
{
    int value = 0;

    readDword(p_comm, GC_REVISION_ID, (unsigned long *)&value);

    return value;
}

/******************************************************************************
 *
 * ARTM_ReadLOS()
 *
 ******************************************************************************/
int
ARTM_ReadLOS(const CX_COMMUNICATION *p_comm, int ch_num)
{
    unsigned long LOSS_OF_SIGNAL = 0;

    ReadBitField(p_comm, AMS_CH0_RXREG18+(0x80*ch_num), 0,0, &LOSS_OF_SIGNAL);

    return (LOSS_OF_SIGNAL);
}

/******************************************************************************
 *
 * ARTM_RasterEngineDisable()
 *
 ******************************************************************************/
void ARTM_RasterEngineDisable(const CX_COMMUNICATION *p_comm, int ch_num)
{
    if(ARTM_BLUE_FLD)
        writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x2F00); // Disable Raster Engine to allow EQ DC/FN to run in auto mode
    else
        writeDword(p_comm, RE0_CSR + (0x1000*ch_num), 0x1F00); // Disable Raster Engine to allow EQ DC/FN to run in auto mode
}

int
ARTM_RasterEngine_Format_Detected(const CX_COMMUNICATION *p_comm, int ch_num)
{
    int fmt_index = 0;

    ReadBitField(p_comm, RE0_DRF_0 + (ch_num*0x1000), 12, 8, (unsigned long *)&fmt_index);

    return fmt_index;
}