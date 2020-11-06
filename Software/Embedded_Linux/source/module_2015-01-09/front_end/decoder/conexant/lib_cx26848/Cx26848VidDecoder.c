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

#include "Cx26848VidDecoder.h"
#include "Cx26848Registers.h"
#include "Cx26848Colibri.h"
#include "Comm.h"

#define CX2684X_VIDEO_LOSS_BLACK    1

static unsigned char _xtal_freq_clk = COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz;

/***********************************************************************************/
/* Forward declarations                                                            */
/***********************************************************************************/
static int CX2684x_initNTSC(const CX_COMMUNICATION *p_communication, unsigned char decoder);
static int CX2684x_initPAL(const CX_COMMUNICATION *p_communication, unsigned char decoder);
static int CX2684x_initEffioNTSC(const CX_COMMUNICATION *p_communication, unsigned char decoder);
static int CX2684x_initEffioPAL(const CX_COMMUNICATION *p_communication, unsigned char decoder);
static int CX2684x_setAudioADC_48K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioADC_44K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioADC_32K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioADC_16K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioADC_8K(const CX_COMMUNICATION *p_comm);

static int CX2684x_setAudioDAC_48K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioDAC_44K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioDAC_32K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioDAC_16K(const CX_COMMUNICATION *p_comm);
static int CX2684x_setAudioDAC_8K(const CX_COMMUNICATION *p_comm);

static int CX2684x_InitVPE(const CX_COMMUNICATION *p_comm,
                           const unsigned char strip_invalid_pixels);

static int CX2684x_setVideoPackEn(const CX_COMMUNICATION *p_comm,
                                  const unsigned long vpe_sel,
                                  const unsigned long src_sel,
                                  const unsigned long vpe_id,
                                  const unsigned long id_len,
                                  const unsigned long strip_en);

static int CX2684x_setVideoPackExtend(const CX_COMMUNICATION *p_comm,
                                      const unsigned long vpe_sel,
                                      const unsigned long video_extend_mode);

static int CX2684x_setVideoPackFormat(const CX_COMMUNICATION *p_comm,
                                      const unsigned long vpe_sel,
                                      const unsigned long standard);

inline unsigned char getBit(unsigned long sample, unsigned char index)
{
    return (unsigned char) ((sample >> index) & 1);
}

inline unsigned long clearBitAtPos(unsigned long value, unsigned char bit)
{
    return value & ~(1 << bit);
}

inline unsigned long setBitAtPos(unsigned long sample, unsigned char bit)
{
    sample |= (1 << bit);
    return sample;
}
/* LPF Coeff */
static unsigned long LPF_Coeff[4][4] =
{
    /*  Coeff0/1    Coeff2/3    Coeff4/5    Coeff6/7 */
    {  0xFFE9FFFC, 0x00330000, 0xFFA4FFCF, 0x013F0135  }, /* LPF 7 */
    {  0x000D000E, 0x001EFFDE, 0xFF76001D, 0x015F00F7  }, /* LPF 8 */
    {  0x000FFFE8, 0xFFDB0001, 0xFF6E005A, 0x01A800BE  }, /* LPF 9 */
    {  0xFFF6FFF3, 0xFFC6001D, 0xFF83005C, 0x01C10094  }, /* LPF 10 */
};
#if 0
/* EQ UTP Coeff for D1 */
static unsigned long EQ_UTP_Coeff_D1[15][4] =
{
    /*  Coeff0/1    Coeff2/3    Coeff4/5    Coeff6/7 */
    {  0xfff6ffef, 0xffefffe1, 0xffe6ffbb, 0x4a5fe3b  }, /* 200 meters */
    {  0xfff5ffe9, 0xffefffd5, 0xfff0ff9b, 0x5cffd4c  }, /* 250 meters */
    {  0xfff6ffe0, 0xfff3ffc5, 0xaff72,    0x74bfc06  }, /* 300 meters */
    {  0xfffaffd5, 0xfffeffaf, 0x3dff3a,   0x9337a49  }, /* 350 meters */
    {  0x2ffc6,    0x12ff92,   0x97feee,   0xba4f7ef  }, /* 400 meters */
    {  0x11ffb2,   0x35ff6b,   0x12afe85,  0xec974c1  }, /* 450 meters */
    {  0x29ff96,   0x6cff35,   0x211fdf3,  0x12d6f078 }, /* 500 meters */
    {  0x4fff70,   0xc3feea,   0x36ffd28,  0x1812eab5 }, /* 550 meters */
    {  0x88ff3a,   0x146fe82,  0x579fc0c,  0x1ed962f7 }, /* 600 meters */
    {  0xdefeef,   0x209fdef,  0x877fa7d,  0x27a1d895 }, /* 650 meters */
    {  0x15cfe85,  0x327fd21,  0xccdf84b,  0x3309caad }, /* 700 meters */
    {  0x214fdef,  0x4c6fbff,  0x1306f535, 0x41e03810 }, /* 750 meters */
    {  0x31ffd1d,  0x71efa65,  0x1be1f0dd, 0x55349f2b }, /* 800 meters */
    {  0x49dfbf3,  0xa79f824,  0x2866eac2, 0x6e6c7de6 }, /* 850 meters */
    {  0x6c0fa4f,  0xf41f4f5,  0x39ffe22f, 0x8f5e5174 }  /* 900 meters */
};

/* EQ UTP Coeff for Effio 960H */
static unsigned long EQ_UTP_Coeff_960H[15][4] =
{
    /*  Coeff0/1    Coeff2/3    Coeff4/5    Coeff6/7 */
    {  0xfff5ffeb, 0xfff1ffdb, 0xffefffab, 0x55a7db0  }, /* 200 meters */
    {  0xfff6ffe1, 0xfff5ffc9, 0x9ff7f,    0x6f4fc56  }, /* 250 meters */
    {  0xfffbffd4, 0x2ffb2,    0x43ff41,   0x9167a67  }, /* 300 meters */
    {  0x5ffc2,    0x1bff90,   0xb0fee8,   0xbee77a6  }, /* 350 meters */
    {  0x18ffa9,   0x49ff5f,   0x16efe66,  0xfbc73bf  }, /* 400 meters */
    {  0x3aff84,   0x97ff18,   0x2aafda7,  0x14d7ee3d }, /* 450 meters */
    {  0x73ff4d,   0x118feaf,  0x4a5fc8d,  0x1bb6e67b }, /* 500 meters */
    {  0xcefefd,   0x1e5fe14,  0x7bffaeb,  0x24fc5b94 }, /* 550 meters */
    {  0x15cfe86,  0x325fd2c,  0xc87f87d,  0x3184cc47 }, /* 600 meters */
    {  0x23afdd3,  0x513fbd3,  0x13caf4df, 0x4280b6d7 }, /* 650 meters */
    {  0x38dfcc9,  0x806f9cf,  0x1eb5ef7e, 0x599118d7 }, /* 700 meters */
    {  0x592fb3b,  0xc7ff6cd,  0x2efbe782, 0x78f16ee1 }, /* 750 meters */
    {  0x89ff8ea,  0x133af251, 0x470ddba9, 0xa3b6343b }, /* 800 meters */
    {  0xd35f575,  0x1d52eba3, 0x6a74ca20, 0xde1ae24c }, /* 850 meters */
    {  0x1414f04e, 0x2c60e1b4, 0x9e41b03a, 0x7fffffff }  /* 900 meters */
};

/* EQ Coaxial Coeff for D1 */
static unsigned long EQ_Coax_Coeff_D1[15][4] =
{
    /*  Coeff0/1    Coeff2/3    Coeff4/5    Coeff6/7 */
    {  0xfffbfffa, 0xfff7fff5, 0xffeeffe9, 0x2e17f7c  }, /* 200 meters */
    {  0xfffafff8, 0xfff5fff2, 0xffeaffe2, 0x329ff4c  }, /* 250 meters */
    {  0xfff9fff7, 0xfff3ffef, 0xffe7ffda, 0x37a7f15  }, /* 300 meters */
    {  0xfff8fff5, 0xfff2ffeb, 0xffe5ffd0, 0x3d3fed5  }, /* 350 meters */
    {  0xfff7fff2, 0xfff1ffe7, 0xffe5ffc6, 0x437fe8c  }, /* 400 meters */
    {  0xfff6fff0, 0xfff0ffe2, 0xffe6ffbb, 0x4a6fe37  }, /* 450 meters */
    {  0xfff6ffed, 0xfff0ffdd, 0xffe8ffae, 0x5227dd5  }, /* 500 meters */
    {  0xfff6ffea, 0xfff0ffd7, 0xffeeffa0, 0x5ac7d64  }, /* 550 meters */
    {  0xfff6ffe7, 0xfff1ffd1, 0xfff6ff8f, 0x6467ce3  }, /* 600 meters */
    {  0xfff7ffe3, 0xfff3ffc9, 0x3ff7d,    0x6f1fc4e  }, /* 650 meters */
    {  0xfff8ffdf, 0xfff7ffc1, 0x13ff67,   0x7b17ba3  }, /* 700 meters */
    {  0xfffaffda, 0xfffbffb8, 0x2aff4f,   0x8877ae0  }, /* 750 meters */
    {  0xfffcffd4, 0x2ffad,    0x47ff33,   0x9767a00  }, /* 800 meters */
    {  0xffce,     0xaffa1,    0x6cff13,   0xa81f8ff  }, /* 850 meters */
    {  0x4ffc7,    0x15ff94,   0x9bfeee,   0xbacf7da  }  /* 900 meters */
};

/* EQ Coaxial Coeff for Effio 960H */
static unsigned long EQ_Coax_Coeff_960H[15][4] =
{
    /*  Coeff0/1    Coeff2/3    Coeff4/5    Coeff6/7 */
    {  0xfffafff9, 0xfff7fff4, 0xffedffe6, 0x30f7f61  }, /* 200 meters */
    {  0xfff9fff7, 0xfff5fff0, 0xffeaffdd, 0x369ff24  }, /* 250 meters */
    {  0xfff8fff4, 0xfff3ffec, 0xffe8ffd3, 0x3d07edc  }, /* 300 meters */
    {  0xfff7fff1, 0xfff2ffe7, 0xffe8ffc7, 0x444fe86  }, /* 350 meters */
    {  0xfff6ffee, 0xfff2ffe1, 0xffeaffba, 0x4c7fe22  }, /* 400 meters */
    {  0xfff6ffeb, 0xfff2ffdb, 0xffefffab, 0x55cfdab  }, /* 450 meters */
    {  0xfff6ffe7, 0xfff3ffd4, 0xfff7ff99, 0x6057d1f  }, /* 500 meters */
    {  0xfff7ffe3, 0xfff5ffcc, 0x4ff85,    0x6c4fc7b  }, /* 550 meters */
    {  0xfff8ffde, 0xfff9ffc3, 0x17ff6d,   0x79e7bbb  }, /* 600 meters */
    {  0xfffaffd8, 0xffffffb8, 0x32ff51,   0x8957ad9  }, /* 650 meters */
    {  0xfffdffd2, 0x7ffac,    0x56ff30,   0x9adf9d1  }, /* 700 meters */
    {  0x2ffca,    0x12ff9d,   0x84ff09,   0xaed789c  }, /* 750 meters */
    {  0x8ffc1,    0x20ff8c,   0xc1fedb,   0xc587733  }, /* 800 meters */
    {  0x10ffb6,   0x33ff78,   0x10efea5,  0xdf6758d  }, /* 850 meters */
    {  0x1affaa,   0x4bff60,   0x170fe65,  0xfcdf3a2  }  /* 900 meters */
};
#endif

/******************************************************************************
 *
 * CX2684x_Reset()
 *
 ******************************************************************************/
void
CX2684x_Reset(const CX_COMMUNICATION *p_comm)
{
    int i = 0;

    for(i = 0; i < MAX_DECODERS; i++)
        writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*i), 0x00008000); //Resets video decoder core

    writeDword(p_comm, CX2684x_CHIP_RST, 0xFFFFFF);

}

/******************************************************************************
 *
 * CX2684x_Initialize()
 *
 ******************************************************************************/
int
CX2684x_initialize(const CX_COMMUNICATION *p_comm, int *video_standard)
{
    int result;

    //Not reset audio
    writeDword(p_comm, CX2684x_CHIP_RST, 0xF4FFE6);

    // Initialize Colibri
    result = CX2684x_initColibri(p_comm, _xtal_freq_clk);

    // Initialize decoder
    result = CX2684x_initDecoder(p_comm, video_standard);

    return result;
}

/******************************************************************************
 *
 * CX2684x_InitDecoder()
 *
 ******************************************************************************/
int
CX2684x_initDecoder(const CX_COMMUNICATION *p_comm, int *video_standard)
{
    unsigned long value = 0;
    int ret_val = TRUE;
    int i;
    int _num_decoders = MAX_DECODERS; // hard-coded
    unsigned int id = 0;   // hard-coded
    unsigned long pll8_int_coef = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x12 : 0x9;


    readDword(p_comm, CX2684x_PLL8_INT_FRAC, &value);
    value &= 0x81FFFFFF;
    writeDword(p_comm, CX2684x_PLL8_INT_FRAC , value | (pll8_int_coef << 25));

    ret_val = CX2684x_setVideoStandard(p_comm, video_standard);

    /* set video proc amp */
    for(i = 0; i < _num_decoders; i++)
    {
        if (video_standard[i] == PAL_VIDEO_STD || video_standard[i] == PAL_EFFIO_STD)
        {
            ret_val = CX2684x_setBrightness(p_comm, BRIGHTNESS_DEFAULT, (unsigned char)i);
        }
        else
        {
            ret_val= CX2684x_setBrightness(p_comm, BRIGHTNESS_NTSC_DEFAULT, (unsigned char)i);
        }

        ret_val = CX2684x_setContrast(p_comm, CONTRAST_DEFAULT, (unsigned char)i);
        ret_val = CX2684x_setSharpness(p_comm, SHARPNESS_DEFAULT, (unsigned char)i);
        ret_val = CX2684x_setHue(p_comm, HUE_DEFAULT, (unsigned char)i);
        ret_val = CX2684x_setSaturation(p_comm, SATURATION_DEFAULT, (unsigned char)i);
        readDword(p_comm, CX2684x_VDECA_HTL_CTRL + 0x200 * i, &value);
        value &= ~(0x0000ffff);
        value |= 0x10;
        value |= 0x08 << 8;
        writeDword(p_comm, CX2684x_VDECA_HTL_CTRL + 0x200 * i, value);


        readDword(p_comm, CX2684x_VDECA_SC_LOOP_CTRL + 0x200 * i, &value);
        value &= ~(0xFFFF0000);
        value |= 0x200 << 16;
        writeDword(p_comm, CX2684x_VDECA_SC_LOOP_CTRL + 0x200 * i, value);
    }

    readDword(p_comm, CX2684x_REV_ID, &value);
    value &= ~0x0f000000;
    value |= id << 24;
    writeDword(p_comm, CX2684x_REV_ID , value);

    return ret_val;
}
#if 0
/******************************************************************************
 *
 * CX2684x_getRevId()
 *
 ******************************************************************************/
static int
CX2684x_getRevId(const CX_COMMUNICATION *p_comm)
{
    unsigned long rev_id = 0;
    readDword(p_comm, (unsigned long) CX2684x_DEV_REV_ID, &rev_id);
    rev_id = (rev_id & 0x000F0000) >> 16;

    return rev_id;
}
#endif
/******************************************************************************
 *
 * CX2684x_SetVideoStandard()
 *
 ******************************************************************************/
int
CX2684x_setVideoStandard(const CX_COMMUNICATION *p_comm,
                         int *standard)
{
    int i;
    int status = TRUE;

    for (i = 0; i < MAX_DECODERS; i++)
    {
        switch(standard[i])
        {
            case NTSC_VIDEO_STD:
                status = CX2684x_initNTSC(p_comm, i);
                break;
            case PAL_VIDEO_STD:
                status = CX2684x_initPAL(p_comm, i);
                break;
            case NTSC_EFFIO_STD:
                status = CX2684x_initEffioNTSC(p_comm, i);
                break;
            case PAL_EFFIO_STD:
                status = CX2684x_initEffioPAL(p_comm, i);
                break;
            default:
                break;
        }
    }

    return status;
}

/******************************************************************************
 *
 * CX2684x_InitNTSC()
 *
 ******************************************************************************/
static int
CX2684x_initNTSC(const CX_COMMUNICATION *p_comm, unsigned char decoder)
{
    int ret_val = TRUE;
    unsigned long value = 0;

    /* reset video decoder */
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0x8000);
    p_comm->sleep(2);
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0);

    writeDword(p_comm, CX2684x_VDECA_MODE_CTRL +(0x200*decoder), 0x0005E001);    /* Default value, vid_fmt_sel=4b0001  NTSC-M */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481040DC);    /* Default value, and disable clamp gating(1) */
    writeDword(p_comm, CX2684x_VDECA_LUMA_CTRL +(0x200*decoder), 0x00108000);    /* Set luma range = 1 */
    writeDword(p_comm, CX2684x_VDECA_CHROMA_CTRL +(0x200*decoder), 0x3D008080);  /* Set c_ext_rng_en = 1(bit29) */

    /* Output setting */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 +(0x200*decoder), 0x001006B1);    /* default value, out_mode=2b01 ITU-R BT.656 */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL_NS +(0x200*decoder), 0x001006B1);  /* default value, out_mode=2b01 ITU-R BT.656 */

    writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL +(0x200*decoder), 0x00080000);  /* for NTSC mode, disable line_average: */

    writeDword(p_comm, CX2684x_VDECA_VCR_DET_CTRL +(0x200*decoder), 0xC0F00AB8); /* field_phase_limit = 0xB */
    writeDword(p_comm, CX2684x_VDECA_HTL_CTRL +(0x200*decoder), 0x00000810);
    writeDword(p_comm, CX2684x_VDECA_CRUSH_CTRL +(0x200*decoder), 0xDE1C0402);

    writeDword(p_comm, CX2684x_VDECA_HORIZ_TIM_CTRL +(0x200*decoder), 0x5E2D0079);   /* 0x612D0074, Default value for NTSC, bgdel=0x61,hactive=0x2d0, hblank=0x074 */
    writeDword(p_comm, CX2684x_VDECA_VERT_TIM_CTRL +(0x200*decoder), 0x1E1E001A);    /* 0x1C1E7014Default value for NTSC, v656blank=0x18,vactive=0x1e7, vblank=0x014 */
    writeDword(p_comm, CX2684x_VDECA_VERT_MISC_CTRL +(0x200*decoder), 0x10C000);     /* sync inc/reduce = 0x3F */

#ifdef CX2684X_VIDEO_LOSS_BLACK
    writeDword(p_comm, CX2684x_VDECA_BLUE_FLD_CTRL +(0x200*decoder), 0x00808010);
#endif

    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL +(0x200*decoder), 0x161F1000);
    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL_FAST + (0x200*decoder), 0x801F1000);

    /* COMB FILTER SETTING Below */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x00180127);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10902020);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x1E199A08);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x20082020);

    writeDword(p_comm, CX2684x_VDECA_C2RB_DC_CTRL  +(0x200*decoder), 0x002022D0);
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481060DC);      /* Default value, and disable clamp gating(1) */
    writeDword(p_comm, CX2684x_VDECA_COL_GAIN_CTRL +(0x200*decoder), 0x00000002);  /* 0x00000012 */

    /*Non-Effio bp_percent [31:20]=0x668, bp_loop_gain [9:8]=2, sync_loop_gain [7:6]=2, agc_loop_gain [3:2]=2, dcc_loop_gain [1:0]=2*/
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL3 +(0x200*decoder), 0x66800e8a);

    writeDword(p_comm, CX2684x_VDECA_FLAT_FIELD_DET   +(0x200*decoder), 0x802091C0);      /* for 624MHz */

    // Confirm if we are still need this setting in Alcaeus+
    readDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), &value);
    value &= 0xEFFFFFFF;                                             /* clear bit 28, disable 12db */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), value);

    readDword(p_comm, CX2684x_VDECA_MAN_AGC_CTRL+(0x200*decoder), &value);
    value |= 0x80000000;
    writeDword(p_comm, CX2684x_VDECA_MAN_AGC_CTRL+(0x200*decoder), value);

    if ( _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_48MHz )
    {
        readDword(p_comm, CX2684x_AFE_DIAG_CTRL+(0x200*decoder), &value);
        value |= 0x00000100;
        writeDword(p_comm, CX2684x_AFE_DIAG_CTRL   +(0x200*decoder), value);      /* Colibri 624MHz mode enable */

        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x2221);
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1E10C1A9);
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x43E2B314);

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x79300594);     /* sync inc/reduce = 0x3F */
    }
    else
    {
        /*Non-Effio eq_ena [16]=0, fifo_size [13:10]=8, src_decim=0x21f [9:0] */
        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x0000221f);          /* Different for 624MHz=0x221, 720MHz=0x21f */
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1E2BE2FE);           /* Go back to 108MHz, for 624MHz = 0x1e2be2fe */
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x43E00000);     /* Default value for NTSC, sc_step=0x43E00000 */

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x903005D4);     /* sync inc/reduce = 0x3F */
    }

    return ret_val;

}

/******************************************************************************
 *
 * CX2684x_InitPAL()
 *
 ******************************************************************************/
static int
CX2684x_initPAL(const CX_COMMUNICATION *p_comm, unsigned char decoder)
{
    int ret_val = TRUE;
    unsigned long   value = 0;

    /* reset video decoder */
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0x8000);
    p_comm->sleep(2);
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0);

    writeDword(p_comm, CX2684x_VDECA_MODE_CTRL +(0x200*decoder), 0x0005E004);    /* Default value, vid_fmt_sel=4'b0100  PAL-BDGHI */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481040DC);    /* Default value, and disable clamp gating(1) */
    writeDword(p_comm, CX2684x_VDECA_LUMA_CTRL +(0x200*decoder), 0x00108000);    /* Set luma range = 1 */
    writeDword(p_comm, CX2684x_VDECA_CHROMA_CTRL +(0x200*decoder), 0x3D008080);  /* Set c_ext_rng_en = 1(bit29) */

    /* Output setting */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 +(0x200*decoder), 0x001006B1);    /* default value, out_mode=2b01 ITU-R BT.656 */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL_NS +(0x200*decoder), 0x001006B1);  /* default value, out_mode=2b01 ITU-R BT.656 */

    writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL +(0x200*decoder), 0x01080000); /* for PAL mode, enable line_average: */

    writeDword(p_comm, CX2684x_VDECA_VCR_DET_CTRL +(0x200*decoder), 0xC0F00AB8); /* field_phase_limit = 0xB, it make vcr_detect bit more reliable */
    writeDword(p_comm, CX2684x_VDECA_HTL_CTRL +(0x200*decoder), 0x00000810);
    writeDword(p_comm, CX2684x_VDECA_CRUSH_CTRL +(0x200*decoder), 0xDE1C0402);

    writeDword(p_comm, CX2684x_VDECA_VERT_TIM_CTRL +(0x200*decoder), 0x26240022);  /* 0x26240022 */
    writeDword(p_comm, CX2684x_VDECA_HORIZ_TIM_CTRL +(0x200*decoder), 0x672D0085); /* bgdel=0x5D,hactive=0x2d0, hblank=0x084 */
    writeDword(p_comm, CX2684x_VDECA_VERT_MISC_CTRL +(0x200*decoder), 0x10C000);   /* sync inc/reduce = 0x3F */

#ifdef CX2684X_VIDEO_LOSS_BLACK
    writeDword(p_comm, CX2684x_VDECA_BLUE_FLD_CTRL +(0x200*decoder), 0x00808010);
#endif

    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL +(0x200*decoder), 0x161F1000);
    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL_FAST + (0x200*decoder), 0x801F1000);

    /* COMB FILTER SETTING Below */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x0001D326); /* 1D727 for RhodeSchwardz disc; 1C727 for AVIA disc */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10900C90); /* default setting; 1e590890 for AVIA disc */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x30061014); /* default setting */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E190000); /* default of 20542030 has more ringing. This setting has a little more dots */
    writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060); /* default of 20542030 has more ringing. This setting has a little more dots */

    /* Macrovision Setting  */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL1 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL2 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL3 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */

    writeDword(p_comm, CX2684x_VDECA_C2RB_DC_CTRL  +(0x200*decoder), 0x002022D0);
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481060DC);
    writeDword(p_comm, CX2684x_VDECA_COL_GAIN_CTRL +(0x200*decoder), 0x00000002); /* 0x00000012 */

    //Non-Effio bp_percent [31:20]=0x668, bp_loop_gain [9:8]=2, sync_loop_gain [7:6]=2, agc_loop_gain [3:2]=2, dcc_loop_gain [1:0]=2
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL3 +(0x200*decoder), 0x66800e8a);
    writeDword(p_comm, CX2684x_VDECA_FLAT_FIELD_DET   +(0x200*decoder), 0x4021C1C0);


    // Confirm if we are still need this setting in Alcaeus+
    readDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), &value);
    value &= 0xEFFFFFFF;                                             /* clear bit 28, disable 12db */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), value);

    if ( _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_48MHz )
    {
        readDword(p_comm, CX2684x_AFE_DIAG_CTRL+(0x200*decoder), &value);
        value |= 0x00000100;
        writeDword(p_comm, CX2684x_AFE_DIAG_CTRL   +(0x200*decoder), value);      /* Colibri 624MHz mode enable */

        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x2221);
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1E10C1A9);
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x54153AC9);

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x80300594);     /* use bp_avg_delay=0x80 and bp_ave_en=1. If complain about black level fluctuate 3code, set bp_ave_en=0 with be_avg_delay=0x50 */
    }
    else
    {
        //Non-Effio eq_ena [16]=0, fifo_size [13:10]=8, src_decim=0x21f [9:0]
        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x0000221f);          /* Different for 624MHz=0x00020221, 720MHz=0x0002021F */
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1E2BE2FE);           /* Go back to 108MHz, for 624MHz = 0x1e2be2fe */
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x5411E2D0);     /* For PAL, 624MHz=0x541280B0, 720MHz=0x5411E2D0*/

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x803005D4);     /* use bp_avg_delay=0x80 and bp_ave_en=1. If complain about black level fluctuate 3code, set bp_ave_en=0 with be_avg_delay=0x50 */
    }

    return ret_val;

}

/******************************************************************************
 *
 * CX2684x_initEffioNTSC()
 *
 ******************************************************************************/
static int
CX2684x_initEffioNTSC(const CX_COMMUNICATION *p_comm, unsigned char decoder)
{
    int ret_val = TRUE;
    unsigned long value = 0;

    /* reset video decoder */
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0x8000);
    p_comm->sleep(2);
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0);

    writeDword(p_comm, CX2684x_VDECA_MODE_CTRL +(0x200*decoder), 0x0005E101);    /* Default value, vid_fmt_sel=4b0001  NTSC-M, effio mode = bit[8] */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481040DC);    /* Default value, and disable clamp gating(1) */
    writeDword(p_comm, CX2684x_VDECA_LUMA_CTRL +(0x200*decoder), 0x00108000);    /* Set luma range = 1 */
    writeDword(p_comm, CX2684x_VDECA_CHROMA_CTRL +(0x200*decoder), 0x3D008080);  /* Set c_ext_rng_en = 1(bit29) */

    /* Output setting */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 +(0x200*decoder), 0x001006B1);    /* default value, out_mode=2b01 ITU-R BT.656 */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL_NS +(0x200*decoder), 0x001006B1);  /* default value, out_mode=2b01 ITU-R BT.656 */

    writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL +(0x200*decoder), 0x00080000); /* for NTSC mode, disable line_average: */

    writeDword(p_comm, CX2684x_VDECA_VCR_DET_CTRL +(0x200*decoder), 0xC0F00AB8); /* field_phase_limit = 0xB */
    writeDword(p_comm, CX2684x_VDECA_HTL_CTRL +(0x200*decoder), 0x00000810);
    writeDword(p_comm, CX2684x_VDECA_CRUSH_CTRL +(0x200*decoder), 0xDE1C0402);    /* Sync inc/reduce = 0x3F */

    writeDword(p_comm, CX2684x_VDECA_VERT_MISC_CTRL +(0x200*decoder), 0x0010C000);    /* Sync inc/reduce = 0x3F */

    writeDword(p_comm, CX2684x_VDECA_HORIZ_TIM_CTRL +(0x200*decoder), 0x813C00A1);/* Effio bgdel=0x81 bit[31:24] ,hactive=0x3c0 bit[21:12], hblank=0x09b bit[9:0] */
    writeDword(p_comm, CX2684x_VDECA_VERT_TIM_CTRL +(0x200*decoder), 0x1E1E001A); /* Effio v656blank_cnt =0x18 bit[31:24], vactive_cnt =0x1e7 bit[21:12], vblank_cnt =0x14  bit[9:0] */

#ifdef CX2684X_VIDEO_LOSS_BLACK
    writeDword(p_comm, CX2684x_VDECA_BLUE_FLD_CTRL +(0x200*decoder), 0x00808010);
#endif

    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL +(0x200*decoder), 0x161F1000);
    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL_FAST + (0x200*decoder), 0x801F2800);

    /* COMB FILTER SETTING Below */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x00180127);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10902020);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x1E199A08);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x20082020);

    writeDword(p_comm, CX2684x_VDECA_C2RB_DC_CTRL  +(0x200*decoder), 0x002022D0);
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481060DC);
    writeDword(p_comm, CX2684x_VDECA_COL_GAIN_CTRL +(0x200*decoder), 0x00000002);  /* 0x00000012 */


    /*Non-Effio bp_percent [31:20]=8e8, bp_loop_gain [9:8]=2, sync_loop_gain [7:6]=2, agc_loop_gain [3:2]=2, dcc_loop_gain [1:0]=2*/
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL3 +(0x200*decoder), 0x8e800e8a);
    writeDword(p_comm, CX2684x_VDECA_FLAT_FIELD_DET   +(0x200*decoder), 0x8020B1C0);


    // Confirm if we are still need this setting in Alcaeus+
    readDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), &value);
    value &= 0xEFFFFFFF;                                             /* clear bit 28, disable 12db */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), value);

    if ( _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_48MHz )
    {
        readDword(p_comm, CX2684x_AFE_DIAG_CTRL+(0x200*decoder), &value);
        value |= 0x00000100;
        writeDword(p_comm, CX2684x_AFE_DIAG_CTRL   +(0x200*decoder), value);      /* Colibri 624MHz mode enable */

        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x222B);
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1D89D89E);
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x32F08ECB);

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x79300594);      /* Sync inc/reduce = 0x3F */
    }
    else
    {
        /*Non-Effio eq_ena [16]=0, fifo_size [13:10]=8, src_decim=0x230 [9:0]*/
        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x00002230);           /* Different for 624MHz=0x0002022B, 720MHz=0x00020230 */
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0xe141d45b);            /* Set to 144MHz */
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x32e8b9c2);      /* Effio **Different for 624MHz=0x32f23723, 720MHz=0x32e8b9c2 */

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x903005D4);      /* Sync inc/reduce = 0x3F */
    }

    return ret_val;

}

/******************************************************************************
 *
 * CX2684x_InitEffioPAL()
 *
 ******************************************************************************/
static int
CX2684x_initEffioPAL(const CX_COMMUNICATION *p_comm, unsigned char decoder)
{
    int ret_val = TRUE;
    unsigned long   value = 0;
    /* reset video decoder */
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0x8000);
    p_comm->sleep(2);
    writeDword(p_comm, CX2684x_VDECA_SOFT_RST_CTRL+(0x200*decoder), 0);

    writeDword(p_comm, CX2684x_VDECA_MODE_CTRL +(0x200*decoder), 0x0005E104);    /* Default value, vid_fmt_sel=4'b0100  PAL-BDGHI, effio mode = bit[8] */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481040DC);    /* Default value, and disable clamp gating(1) */
    writeDword(p_comm, CX2684x_VDECA_LUMA_CTRL +(0x200*decoder), 0x00108000);    /* Set luma range = 1 */
    writeDword(p_comm, CX2684x_VDECA_CHROMA_CTRL +(0x200*decoder), 0x3D008080);  /* Set c_ext_rng_en = 1(bit29) */

    /* Output setting */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 +(0x200*decoder), 0x001006B1);    /* default value, out_mode=2b01 ITU-R BT.656 */
    writeDword(p_comm, CX2684x_VDECA_OUT_CTRL_NS +(0x200*decoder), 0x001006B1);  /* default value, out_mode=2b01 ITU-R BT.656 */

    writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL +(0x200*decoder), 0x01080000); /* for PAL mode, enable line_average: */

    writeDword(p_comm, CX2684x_VDECA_VCR_DET_CTRL +(0x200*decoder), 0xC0F00AB8); /* field_phase_limit = 0xB, it make vcr_detect bit more reliable */
    writeDword(p_comm, CX2684x_VDECA_HTL_CTRL +(0x200*decoder), 0x00000810);
    writeDword(p_comm, CX2684x_VDECA_CRUSH_CTRL +(0x200*decoder), 0xDE1C0402);

    writeDword(p_comm, CX2684x_VDECA_VERT_TIM_CTRL +(0x200*decoder), 0x28240024);  /* Effio v656blank_cnt =0x26 bit[31:24], vactive_cnt =0x240 bit[21:12], vblank_cnt =0x022  bit[9:0] */
    writeDword(p_comm, CX2684x_VDECA_HORIZ_TIM_CTRL +(0x200*decoder), 0x843C00B2); /* Effio bgdel=0x84 bit[31:24] ,hactive=0x3c0 bit[21:12], hblank=0x0a6 bit[9:0] */

    /* Added to Cx2684x */
    writeDword(p_comm, CX2684x_VDECA_VERT_MISC_CTRL +(0x200*decoder), 0x10C000);

#ifdef CX2684X_VIDEO_LOSS_BLACK
    writeDword(p_comm, CX2684x_VDECA_BLUE_FLD_CTRL +(0x200*decoder), 0x00808010);
#endif

    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL +(0x200*decoder), 0x161F1000);
    writeDword(p_comm, CX2684x_VDECA_PLL_CTRL_FAST + (0x200*decoder), 0x801F2800);

    /* COMB FILTER SETTING Below */
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x0001D326);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10900C90);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x30061014);
    writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E190000);
    writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);

    /* Macrovision Setting  */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL1 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL2 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */
    writeDword(p_comm, CX2684x_VDECA_MV_DT_CTRL3 +(0x200*decoder), 0x00000000);  /* Default value for PAL Macrovision */

    writeDword(p_comm, CX2684x_VDECA_C2RB_DC_CTRL  +(0x200*decoder), 0x002022D0);
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL2 +(0x200*decoder), 0x481060DC);
    writeDword(p_comm, CX2684x_VDECA_COL_GAIN_CTRL +(0x200*decoder), 0x00000002); /* 0x00000012 */


    //Non-Effio bp_percent [31:20]=0x8e8, bp_loop_gain [9:8]=2, sync_loop_gain [7:6]=2, agc_loop_gain [3:2]=2, dcc_loop_gain [1:0]=2
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL3 +(0x200*decoder), 0x8e800e8a);
    writeDword(p_comm, CX2684x_VDECA_FLAT_FIELD_DET   +(0x200*decoder), 0x8021C1C0);

    // Confirm if we are still need this setting in Alcaeus+
    readDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), &value);
    value &= 0xEFFFFFFF;                                                 /* clear bit 28, disable 12db */
    writeDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*decoder), value);

    if ( _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_48MHz )
    {
        readDword(p_comm, CX2684x_AFE_DIAG_CTRL+(0x200*decoder), &value);
        value |= 0x00000100;
        writeDword(p_comm, CX2684x_AFE_DIAG_CTRL   +(0x200*decoder), value);      /* Colibri 624MHz mode enable */

        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x222B);
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0x1D89D89E);
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x3F1803A0);

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x80300594);
    }
    else
    {
        /* Non-Effio eq_ena [16]=0, fifo_size [13:10]=8, src_decim=0x230 [9:0] */
        writeDword(p_comm, CX2684x_VDECA_SRC_CFG +(0x200*decoder), 0x00002230);        /* Different for 624MHz=0x0002022B, 720MHz=0x00020230 */
        writeDword(p_comm, CX2684x_PLL0_INT_FRAC + (0x8*decoder), 0xe141d45b);         /* Set to 144MHz */
        writeDword(p_comm, CX2684x_VDECA_SC_STEP_SIZE +(0x200*decoder), 0x3f0d6a1c);   /* For Effio PAL **Different for 624MHz=0x3f178d60, 720MHz=0x3f0d6a1c */

        writeDword(p_comm, CX2684x_VDECA_BP_MISC_CTRL +(0x200*decoder), 0x803005D4);
    }

    return ret_val;
}

/******************************************************************************
 *
 * CX2684x_InitAudioChannels()
 *
 ******************************************************************************/
int
CX2684x_initAudioChannels(const CX_COMMUNICATION *p_comm)
{
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /*** Setup ADC 48KHz 16bit ***/
    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC); /* Default Value */
    writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val); /* Power on PLL, PLL frequency = 98.304MHz */

    /* ADC */
    writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00026666); /* bit[22:20]Aadc_sample_rate = 00 (48KHz), Disable and Mute ADC1,2,3,4 and 5 */
    writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default)  */
    writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00011111); /* 48KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* Set record port (I2S 16 Channels, 16bit, Master) output ADCs data */
    /* Set playback port (I2S 2 Channels, 16bit, Master) to DAC */
    writeDword(p_comm, CX2684x_AP_CFG0, 0x01050135);  /* record(16chs) and playback(2chs) ports: i2s_mode, 1bit delay, 16bit data */
    //writeDword(p_comm, CX2684x_AP_CFG1, 0x239C1505);  /* record and playback ports are master, mix/cascade in: 16bit data,*/
    writeDword(p_comm, CX2684x_AP_CFG2, 0x400000A0);  /* bit[30:28]fs_rate=100 (48KHz), bit[8:4]aout_sel=A (audio_pb to playback) */
    writeDword(p_comm, CX2684x_AP_CFG2, 0x408000A0);  /* bit[30:28]fs_rate=100 (48KHz), bit[8:4]aout_sel=A (audio_pb to playback), Enable audio processor */
    writeDword(p_comm, CX2684x_AP_CFG3, 0x0B800701);  /* record-sdat output, bclk = 1536KHz(48K*16bit*2channels) for playback, 12.288MHz for record */


    /*** Setup DAC ***/
    writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000230); // Power up AudioDAC

    return TRUE;
}

/******************************************************************************
 *
 * CX2684x_setDACVolume()
 *
 ******************************************************************************/
int
CX2684x_setDACVolume(const CX_COMMUNICATION *p_comm,
                  const unsigned long volume)
{
    int result = TRUE;
    unsigned long value = 0;

    /* set the volume bits 13:8*/
    result = readDword(p_comm, (unsigned long) CX2684x_AUD_GAIN2, &value);
    value &= 0xFFFFC0FF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AUD_GAIN2, value | (volume << 8));

    return result;
}

/******************************************************************************
 *
 * CX2684x_getDACVolume()
 *
 ******************************************************************************/
unsigned long
CX2684x_getDACVolume(const CX_COMMUNICATION *p_comm)
{
    unsigned long volume = 0;
    readDword(p_comm, (unsigned long) CX2684x_AUD_GAIN2, &volume);
    volume &= 0x00003F00;
    volume >>= 8;

    return volume;
}

/******************************************************************************
 *
 * CX2684x_setupAudioADCInput()
 *
 ******************************************************************************/
int
CX2684x_setupAudioADCInput(const CX_COMMUNICATION *p_comm,
                        const unsigned long sampling_rate)
{
    int result = TRUE;

    switch(sampling_rate)
    {
        case 44100:
            result = CX2684x_setAudioADC_44K(p_comm);
            break;
        case 32000:
            result = CX2684x_setAudioADC_32K(p_comm);
            break;
        case 16000:
            result = CX2684x_setAudioADC_16K(p_comm);
            break;
        case 8000:
            result = CX2684x_setAudioADC_8K(p_comm);
            break;
        case 48000:
        default:
            result = CX2684x_setAudioADC_48K(p_comm);
            break;
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_setupI2SCascade()
 *
 ******************************************************************************/
int
CX2684x_setupI2SCascade(const CX_COMMUNICATION *p_comm,
                     const unsigned long data_width,
                     const unsigned long bit_delay,
                     const unsigned long cascade_direction)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long cas_oe = (cascade_direction == CASCADE_DIR_INPUT) ? 0 : 1;

    /* Disable Audio Processor before start of configuration */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value & 0xFF7FFFFF);

    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG1, &value);
    value &= 0x9FFFFFC3;
    value |= (data_width << 2);  /* audio data width for record port */
    value |= (bit_delay  << 4);  /* bit delay for record port */
    value |= (cas_oe     << 30); /* direction of CAS in port  */
    value |= (0x1        << 29); /* set to msb mode           */
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG1, value);

    /* not delay */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value & 0xFFBFFFFF);

    /* to compensate the delay  */
    result = readDword(p_comm, (unsigned long) CX2684x_AUD_CAP_DLY, &value);
    value &= 0xFFE00FFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AUD_CAP_DLY, value | 0x7000);

    /* SB Mode is always normal */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG3, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG3, value & 0xFFFEFFFF);

    /* Re-enable Audio Processor after configuration */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x800000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioADC_48K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioADC_48K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* *** Setup ADC 48KHz 16bit *** */
    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC);  /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);      /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00026666); /* bit[22:20]Aadc_sample_rate = 00 (48KHz), Disable and Mute ADC1,2,3,4 and 5 */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00011111); /* 48KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* 48K audio sample rate */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0x8FFFFFFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x40000000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioADC_44K()
 *
 ******************************************************************************/
int
CX2684x_setAudioADC_44K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 11.2896)/6 = 90.3168 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x16944673);  /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);      /* Power on PLL, PLL frequency = 90.3168MHz */

    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00426666); /* bit[22:20]Aadc_sample_rate = 100 (44.1KHz), Disable and Mute ADC1,2,3,4 and 5 */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00411111); /* 44.1KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* 44K audio sample rate */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0x8FFFFFFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x50000000);

    return result;
}


/******************************************************************************
 *
 * CX2684x_setAudioADC_32K()
 *
 ******************************************************************************/
int
CX2684x_setAudioADC_32K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC);  /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);      /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00126666); /* bit[22:20]Aadc_sample_rate = 001 (32KHz), Disable and Mute ADC1,2,3,4 and 5 */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00111111); /* 32KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* 32K audio sample rate */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0x8FFFFFFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x20000000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioADC_16K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioADC_16K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC);  /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);      /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00226666); /* bit[22:20]Aadc_sample_rate = 010 (16KHz), Disable and Mute ADC1,2,3,4 and 5 */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00211111); /* 16KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* 16K audio sample rate */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0x8FFFFFFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x10000000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioADC_8K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioADC_8K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC);  /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);      /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00326666); /* bit[22:20]Aadc_sample_rate = 011 (8KHz), Disable and Mute ADC1,2,3,4 and 5 */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL2, 0x00000000); /* Default value, bit[13:12] aadc_digital_gain = 00 (4dB,default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL1, 0xF1005555); /* Power on ADCs 1-8, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL2, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL3, 0x00000000); /* Default value */
    result &= writeDword(p_comm, CX2684x_AADC_ANA_CTRL4, 0x00000012); /* Power on ADC 9, PGA gain=+9dB (default) */
    result &= writeDword(p_comm, CX2684x_AADC_DIG_CTRL1, 0x00311111); /* 8KHz Sample Rate, Enable ADC1,2,3,4 and 5 */

    /* 8K audio sample rate */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0x8FFFFFFF;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setupAudioDACOutput()
 *
 ******************************************************************************/
int
CX2684x_setupAudioDACOutput(const CX_COMMUNICATION *p_comm,
                         const unsigned long sampling_rate,
                         const unsigned long audio_out_sel,
                         const unsigned long audio_gain_en,
                         const unsigned long master_mode)
{
    int result = TRUE;
    unsigned long value = 0;

    switch(sampling_rate)
    {
        case 44100:
            result = CX2684x_setAudioDAC_44K(p_comm);
            break;
        case 32000:
            result = CX2684x_setAudioDAC_32K(p_comm);
            break;
        case 16000:
            result = CX2684x_setAudioDAC_16K(p_comm);
            break;
        case 8000:
            result = CX2684x_setAudioDAC_8K(p_comm);
            break;
        case 48000:
        default:
            result = CX2684x_setAudioDAC_48K(p_comm);
            break;
    }

    /* set the Playback interface mode to master/slave mode */
    result  = readDword(p_comm, (unsigned long) CX2684x_AP_CFG1, &value);
    value  &= ~(1 << 23);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG1, value | (master_mode << 23) );

    /* choose the source to ADAC */
    result  = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value  &= 0xFFFFFE0F;
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | (audio_out_sel << 4));

    /* enable gain adjustment of DAC bit 2 and bit 17*/
    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    value  &= 0xFFFDFFFB;
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value | ((audio_gain_en << 17) | (audio_gain_en << 2)) );

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioDAC_48K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioDAC_48K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC); /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);     /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000330);    /* Power up AudioDAC */

    /* 48K sample rate */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0x8FFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value | 0x40000000); /* bit[30:28]fs_rate=100 (48KHz) */

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioDAC_44K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioDAC_44K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 11.2896)/6 = 90.3168 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x16944673); /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);     /* Power on PLL, PLL frequency = 90.3168MHz */

    result &= writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000330);    /* Power up AudioDAC */

    /* 44K sample rate */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0x8FFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value | 0x50000000); /* bit[30:28]fs_rate=101 (44KHz) */

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioDAC_32K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioDAC_32K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC); /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);     /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000330);    /* Power up AudioDAC */

    /* 32K sample rate */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0x8FFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value | 0x20000000); /* bit[30:28]fs_rate=010 (32KHz) */

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioDAC_16K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioDAC_16K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC); /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);     /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000330);    /* Power up AudioDAC */

    /* 16K sample rate */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0x8FFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value | 0x10000000); /* bit[30:28]fs_rate=001 (16KHz) */

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioDAC_8K()
 *
 ******************************************************************************/
static int
CX2684x_setAudioDAC_8K(const CX_COMMUNICATION *p_comm)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long pll9_ctrl_val = (_xtal_freq_clk == COLIBRI_FCLK_720MHZ_CRYSTAL_24MHZ || _xtal_freq_clk == COLIBRI_FCLK_624MHZ_CRYSTAL_24MHz) ? 0x00522513 : 0x00522516;

    /* Power on PLL, PLL frequency = (48e6 * 12.288)/6 = 98.304 MHz */
    result &= writeDword(p_comm, CX2684x_PLL9_INT_FRAC, 0x189374BC); /* Default Value */
    result &= writeDword(p_comm, CX2684x_PLL9_CTRL, pll9_ctrl_val);     /* Power on PLL, PLL frequency = 98.304MHz */

    result &= writeDword(p_comm, CX2684x_ADAC_CTRL0, 0x00000330);    /* Power up AudioDAC */

    /* 8K sample rate */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0x8FFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value); /* bit[30:28]fs_rate=0x0 (8KHz) */

    return result;
}

/******************************************************************************
 *
 * CX2684x_setAudioMapSources()
 *
 ******************************************************************************/
static int
CX2684x_setAudioMapSources(const CX_COMMUNICATION *p_comm,
                        unsigned long output_interface,
                        int channel,
                        unsigned long map)
{
    int result = TRUE;
    unsigned long value = 0;
    unsigned long mask  = 0;
    unsigned long reg_addr = CX2684x_AP_RMAP0;
    int lo_bit = (channel - (6 * (channel / 6)) ) * 5;
    int i;

    if ( output_interface == AUDIO_RECORD_OUTPUT )
    {
        reg_addr = CX2684x_AP_RMAP0 + (4 * (channel / 6));  /* for the register address */
    }
    else if ( output_interface == AUDIO_MIX_OUTPUT )
    {
        reg_addr = CX2684x_AP_MMAP0 + (4 * (channel / 6));     /* for the register address */
    }

    for ( i = 0; i <= 4; i++)
    {
        mask |= (0x1 << (lo_bit + i));
    }

    result = readDword(p_comm,   reg_addr, &value);
    value &= ~mask;
    result &= writeDword(p_comm, reg_addr, value | (map << lo_bit) );

    return result;
}

/******************************************************************************
 *
 * CX2684x_configAudioRecord()
 *
 ******************************************************************************/
int
CX2684x_configAudioRecord(const CX_COMMUNICATION *p_comm,
                       const unsigned long     i2s_mode,
                       const unsigned long     master_mode,
                       const unsigned long     data_width,
                       const unsigned long     bit_delay,
                       const unsigned long     num_channels,
                       unsigned long          *map_sources,
                       const unsigned long     compression_mode)
{
    int result          = TRUE;
    int num_map_src     = 0;
    int i               = 0;
    unsigned long value = 0;

    /* Disable Audio Processor before start of configuration */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value & 0xFF7FFFFF);

    result &= readDword(p_comm, CX2684x_AP_CFG0, &value);
    value  &= 0xFFFFCC80;
    value  |= i2s_mode  << 12;                                  /* Set the I2S Mode for Record Port */
    value  |= bit_delay << 8;                                   /* Set the Bit Delay */

    if ( num_channels < AUDIO_PLAYBACK_R_CHANNEL )
        value |= num_channels << 4;                             /* Set the Number of Channels for record Port */
    else
        return FALSE;                                           /* User entered playback channel or other non valid value */

    if ( data_width == AUDIO_DATA_WIDTH_8BIT )
        value |= 0x0;
    else if ( data_width == AUDIO_DATA_WIDTH_16BIT )
        value |= 0x5;
    else if ( data_width == AUDIO_DATA_WIDTH_24BIT )
        value |= 0xA;

    result &= writeDword(p_comm, CX2684x_AP_CFG0, value );

    result &= readDword(p_comm, CX2684x_AP_CFG1, &value);
    value  &= 0xFFE0FFFF;                                       /* wclk&bclk polarity, wclk generated externally */
    value  |= 0x100000;                                         /* Hardcode to First msb    !!!!! */
    value  |= master_mode << 19;                                /* Set the interface Mode to Master or Slave */

    if ( master_mode == AUDIO_MASTER_MODE )
        value |= 0x40000;                                       /* wclk bit[18] generated internally */

    result &= writeDword(p_comm, CX2684x_AP_CFG1, value );

    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0xFFFBF1FF;                                       /* wclk no delay, left adjusted */

    if ( compression_mode == AUDIO_UNCOMPRESSION )
        value &= 0xFFFFFBFF;                                    /* no encoded   */
    else
        value |= ( 0x400 | (compression_mode << 9) );           /* encoded, aLaw or uLaw */

    result &= writeDword(p_comm, CX2684x_AP_CFG2, value );

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    value  &= 0xFFFFDFFF;                                       /* normal mode */
    value  |= 0x1;                                              /* sdat output */
    value  |= 0x40000;                                          /* bclk calcuated by bit_width,fs_rate,and ch_num */
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value );

    /* Map the sources with rMap */
    switch(num_channels)
    {
        case AUDIO_2_CHANNELS :
            num_map_src  = 2;
            break;
        case AUDIO_4_CHANNELS :
            num_map_src  = 4;
            break;
        case AUDIO_8_CHANNELS :
            num_map_src  = 8;
            break;
        case AUDIO_16_CHANNELS :
            num_map_src  = 16;
            break;
        case AUDIO_24_CHANNELS :
            num_map_src  = 24;
            break;
        default:
            num_map_src  = 2;
            break;
    }

    for (i = 0; i < num_map_src; i++)
    {
        CX2684x_setAudioMapSources(p_comm, AUDIO_RECORD_OUTPUT, i, map_sources[i]);
    }

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value & 0xFFFFDFFF); /* Sign Bit Mode. Hardcode to normal */

    /* Re-enable Audio Processor after configuration */
    result &= readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x800000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_configAudioMix()
 *
 ******************************************************************************/
static int
CX2684x_configAudioMix(const CX_COMMUNICATION *p_comm,
                    const unsigned long     i2s_mode,
                    const unsigned long     master_mode,
                    const unsigned long     data_width,
                    const unsigned long     bit_delay,
                    const unsigned long     num_channels,
                    unsigned long          *map_sources,
                    const unsigned long     compression_mode)
{
    int result          = TRUE;
    int num_map_src     = 0;
    int i               = 0;
    unsigned long value = 0;

    /* Disable Audio Processor before start of configuration */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    value &= 0xFE7FFFFF;                                              /* And disable Audio Mixer as well */
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value);

    result &= readDword(p_comm, CX2684x_AP_CFG0, &value);
    value  &= 0xFFFFC380;
    value  |= i2s_mode  << 12;                                  /* Set the I2S Mode for Record Port */
    value  |= bit_delay << 10;                                  /* Set the Bit Delay */
    value  |= num_channels << 4;                                /* Set the Number of Channels for record Port */

    if ( data_width == AUDIO_DATA_WIDTH_8BIT )
        value |= 0x0;
    else if ( data_width == AUDIO_DATA_WIDTH_16BIT )
        value |= 0x5;
    else if ( data_width == AUDIO_DATA_WIDTH_24BIT )
        value |= 0xA;

    result &= writeDword(p_comm, CX2684x_AP_CFG0, value );

    result &= readDword(p_comm, CX2684x_AP_CFG1, &value);
    value  &= 0xFFF0F3FF;                                       /* wclk&bclk polarity, wclk generated externally */
    value  |= data_width << 10;                                 /* set data width */
    value  |= 0x1000;                                           /* msb */
    value  |= master_mode << 19;                                /* Set the interface Mode to Master or Slave */

    if ( master_mode == AUDIO_MASTER_MODE )
        value |= 0x40000;                                       /* wclk bit[18] generated internally */

    result &= writeDword(p_comm, CX2684x_AP_CFG1, value );

    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0xFFFB8FFF;                                       /* wclk no delay, left adjusted */

    if ( compression_mode == AUDIO_UNCOMPRESSION )
        value &= 0xFFFFDFFF;                                    /* no encoded   */
    else
        value |= ( 0x2000 | (compression_mode << 12) );         /* encoded, aLaw or uLaw */

    result &= writeDword(p_comm, CX2684x_AP_CFG2, value );

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    value  &= 0xFFFFDFFF;                                       /* normal mode */
    value  |= 0x2;                                              /* sdat output */
    value  |= 0x40000;                                          /* bclk calcuated based on  bit_width,sample rate,and ch_num */
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value );

    /* Map the sources with rMap */
    switch(num_channels)
    {
        case AUDIO_2_CHANNELS :
            num_map_src  = 2;
            break;
        case AUDIO_4_CHANNELS :
            num_map_src  = 4;
            break;
        case AUDIO_8_CHANNELS :
            num_map_src  = 8;
            break;
        case AUDIO_16_CHANNELS :
            num_map_src  = 16;
            break;
        case AUDIO_24_CHANNELS :
            num_map_src  = 24;
            break;
        default:
            num_map_src  = 2;
            break;
    }

    for (i = 0; i < num_map_src; i++)
    {
        CX2684x_setAudioMapSources(p_comm, AUDIO_MIX_OUTPUT, i, map_sources[i]);
    }

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value & 0xFFFFBFFF); /* Sign Bit Mode. Hardcode to normal */

    /* Re-Enable Audio Mix after configuration */
    result &= readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x1000000);

    /* Re-enable Audio Processor after configuration */
    result &= readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x800000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_configAudioPlayback()
 *
 ******************************************************************************/
int
CX2684x_configAudioPlayback(const CX_COMMUNICATION *p_comm,
                         const unsigned long     i2s_mode,
                         const unsigned long     master_mode,
                         const unsigned long     data_width,
                         const unsigned long     bit_delay,
                         const unsigned long     num_channels,
                         const unsigned long     compression_mode)
{
    int result = TRUE;
    unsigned long value = 0;

    /* Disable Audio Processor before start of configuration */
    result = readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value & 0xFF7FFFFF);

    result &= readDword(p_comm, CX2684x_AP_CFG0, &value);
    value  &= 0xCC80FFFF;                                       /* 0x0 = 2 channels */
    value  |= i2s_mode  << 28;                                  /* Set the I2S Mode for Record Port */
    value  |= bit_delay << 24;                                  /* Set the Bit Delay */

    if ( data_width == AUDIO_DATA_WIDTH_8BIT )
        value &= 0xFFF0FFFF;
    else if ( data_width == AUDIO_DATA_WIDTH_16BIT )
        value |= 0x5 << 16;
    else if ( data_width == AUDIO_DATA_WIDTH_24BIT )
        value |= 0xA << 16;

    result &= writeDword(p_comm, CX2684x_AP_CFG0, value );

    result &= readDword(p_comm, CX2684x_AP_CFG1, &value);
    value  &= 0xFC1FFFFF;                                       /* wclk polarity, bclk polarity */

    if ( (num_channels == AUDIO_PLAYBACK_R_CHANNEL) || (num_channels == AUDIO_PLAYBACK_L_CHANNEL) )
        value |= (num_channels - 0x5) << 25;

    value |= 0x01000000;                                        /* MSB bit[24]*/
    value |= master_mode << 23;

    result &= writeDword(p_comm, CX2684x_AP_CFG1, value );

    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0xFFE57FFF;                                       /* sdat delay = 0, wclk delay = 0, left aligned */

    if ( compression_mode == AUDIO_UNCOMPRESSION )
        value &= 0xFFFEFFFF;                                    /* no encoded   */
    else
        value |= ( 0x10000 | (compression_mode << 15) );        /* encoded, aLaw or uLaw */

    result &= writeDword(p_comm, CX2684x_AP_CFG2, value );

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    value  |= 0x80000;                                          /* bclk is always auto select based on FsRate, datawidth and num ch. */
    value  &= 0xFFFFFFFF;                                       /* sb mode is always normal */
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value);

    /* Re-enable Audio Processor after configuration */
    result &= readDword(p_comm, (unsigned long) CX2684x_AP_CFG2, &value);
    result &= writeDword(p_comm, (unsigned long)CX2684x_AP_CFG2, value | 0x800000);

    return result;
}

/******************************************************************************
 *
 * CX2684x_setupI2SInterface()
 *
 ******************************************************************************/
int
CX2684x_setupI2SInterface(const CX_COMMUNICATION *p_comm,
                       const unsigned long output_interface,
                       const unsigned long i2s_mode,
                       const unsigned long master_mode,
                       const unsigned long data_width,
                       const unsigned long bit_delay,
                       const unsigned long num_channels,
                       unsigned long      *map_sources,
                       const unsigned long compression_mode)
{
    int result = TRUE;
    unsigned long value = 0;

    /* mclk_en is disabled */
    result &= readDword(p_comm, CX2684x_AP_CFG2, &value);
    value  &= 0xFDFFFFFF;
    result &= writeDword(p_comm, CX2684x_AP_CFG2, value);

    result &= readDword(p_comm, CX2684x_AP_CFG3, &value);
    value  &= 0xFFFFFFEF;                                       /* mclk_512  is always 256*fs */
    result &= writeDword(p_comm, CX2684x_AP_CFG3, value | 0x800);  /* polarity of mclk_out bit[11] */

    switch(output_interface)
    {
        case AUDIO_RECORD_OUTPUT:
            result = CX2684x_configAudioRecord(p_comm, i2s_mode, master_mode, data_width, bit_delay,
                                            num_channels, map_sources, compression_mode);
            break;
        case AUDIO_MIX_OUTPUT:
            result = CX2684x_configAudioMix(p_comm, i2s_mode, master_mode, data_width, bit_delay,
                                         num_channels, map_sources, compression_mode);
            break;
        case AUDIO_PLAYBACK:
        default:
            result = CX2684x_configAudioPlayback(p_comm, i2s_mode, master_mode, data_width, bit_delay,
                                              num_channels, compression_mode);
            break;
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_setResolution()
 *
 ******************************************************************************/
int
CX2684x_setResolution(const CX_COMMUNICATION *p_comm,
                   unsigned long width,
                   const unsigned long height,
                   unsigned long decoder)
{
    unsigned long hscale = 0x0;
    unsigned long vscale = 0x0;
    unsigned long value;


    /* validate the width - cannot be negative */
    if(width > MAX_D1_WIDTH)
    {
        width = MAX_D1_WIDTH;
    }

    switch( width )
    {
        case 320:
            hscale = 0x13E34B;
            vscale = 0x0;
            break;

        case 352:
            hscale = 0x10A273;
            vscale = 0x0;
            break;

        case 360:
            hscale = 0x100000;
            vscale = 0x0;
            break;

        case 176:
            hscale = 0x3115B2;
            vscale = 0x1E00;
            break;

        case 160:
            hscale = 0x378D84;
            vscale = 0x1E00;
            break;

        case 480:
            hscale = 0x07f770;
            vscale = 0x0;
            break;

        case 704:
            hscale = 0x5d18;
            break;

        default: /* 720 */
            hscale = 0x0;
            vscale = 0x0;
            break;
    }

    /* write scaling values for each decoders */
    writeDword(p_comm, CX2684x_VDECA_HSCALE_CTRL+(0x200*decoder), hscale);

    readDword(p_comm, CX2684x_VDECA_VSCALE_CTRL + (0x200 * decoder), &value);
    value &= 0xFFFFE000;
    writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL+(0x200*decoder), value | vscale);

    return TRUE;
}

/*******************************************************************************************************
 *
 * CX2684x_setupD1
 * setup Cx2684x to output 1 D1 video of selected channel to dvp_port selected
 *
 *******************************************************************************************************/
int
CX2684x_setupD1(const CX_COMMUNICATION *p_comm,
             const unsigned long channel,
             const unsigned char dvp_port,
             const unsigned long cascade_direction,
             const unsigned char use_scaled_output)
{
    unsigned long camera = channel % 8;
    int result = TRUE;
    unsigned long value;

    if (channel >=  16)
    {
        camera = channel;
    }
    else if(!use_scaled_output)
    {
        camera = (camera * 2) + 1;    // non-scaled path
    }
    else
    {
        camera = (camera * 2);       /* scaled */
    }
    /* [19]    enable video data bus driver                          */
    /* [18]    enable clock pin driver                               */
    /* [17]    indicates polarity of OMUX clock output               */
    /* [12:8]  set video decoder to be outputted on desired dvp port */
    /* [6:4]   set output mode to VDEC passthrough                   */
    /* [3]     enable clk pin driver                                 */
    /* [2]     set bandwidth of stream to 27Mbyte/s                  */
    /* [1]     select video packing engine group low                 */
    /* [0]     disable remux function                                */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x4 * dvp_port), &value);
    value &= 0xFFF1E0FF;
    /*if (cascade_direction == CASCADE_DIR_INPUT)*/
        value |= 0x20000; /* inverted clock when cascading */
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x4 * dvp_port), value | 0xC0010 | (camera << 8));

    return result;
}

/******************************************************************************
 *
 * mapM()
 *
 * Map to decoder register setting
 *
 ******************************************************************************/
static int mapM(
    int  srcMin,
    int  srcMax,
    int  srcVal,
    int  dstMin,
    int  dstMax,
    int* dstVal
)
{
    int numerator;
    int denominator;
    int quotient;

    if((srcMin == srcMax) || (srcVal < srcMin) || (srcVal > srcMax))
    {
        return -1;
    }

    // This is the overall expression used:
    // *dstVal = (srcVal - srcMin)*(dstMax - dstMin) / (srcMax - srcMin) + dstMin;
    // but we need to account for rounding so below we use the modulus
    // operator to find the remainder and increment if necessary.
    numerator   = (srcVal - srcMin)*(dstMax - dstMin);
    denominator = srcMax - srcMin;
    quotient    = numerator/denominator;

    if(2 * ( numerator % denominator ) >= denominator)
    {
        quotient++;
    }

    *dstVal = quotient + dstMin;

    return 0;
}

/******************************************************************************
 *
 * convert_to_twos()
 *
 ******************************************************************************/
static unsigned long convert_to_twos(long numeric, unsigned long bits_len)
{
    unsigned char temp;

    if (numeric >= 0)
        return numeric;
    else
    {
        numeric = numeric * -1;
        temp = ~(numeric & 0xFF);
        temp += 1;
        return temp;
    }
}

/******************************************************************************
 *
 * CX2684x_setBrightness()
 *
 ******************************************************************************/
int
CX2684x_setBrightness(const CX_COMMUNICATION *p_comm,
                   const int brightness,
                   const unsigned char decoder)
{
    int ret_val = 0;
    int val = 0;
    unsigned long value;
    unsigned char byBrightness;

    if((brightness > VIDEO_PROCAMP_MAX) || (brightness < VIDEO_PROCAMP_MIN))
    {
        return FALSE;
    }

    ret_val = mapM(VIDEO_PROCAMP_MIN, VIDEO_PROCAMP_MAX, brightness, SIGNED_BYTE_MIN, SIGNED_BYTE_MAX, &val);
    val = convert_to_twos(val, 8);
    byBrightness = (unsigned char)(val & 0xFF);
    readDword(p_comm, (unsigned long)(CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), &value);
    value &= 0xFFFFFF00;
    writeDword(p_comm, (unsigned long)(CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), value | byBrightness);

    return TRUE;

}

/******************************************************************************
 *
 * CX2684x_setContrast()
 *
 ******************************************************************************/
int
CX2684x_setContrast(const CX_COMMUNICATION *p_comm,
                 const int contrast,
                 const unsigned char decoder)
{
    int ret_val = 0;
    int val = 0;
    unsigned long value;
    unsigned char byContrast;

    if((contrast > VIDEO_PROCAMP_MAX) || (contrast < VIDEO_PROCAMP_MIN))
    {
        return FALSE;
    }

    ret_val = mapM(VIDEO_PROCAMP_MIN, VIDEO_PROCAMP_MAX, contrast, UNSIGNED_BYTE_MIN, UNSIGNED_BYTE_MAX, &val);

    byContrast = (unsigned char)(val & 0xff);

    readDword(p_comm, (CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), &value);
    value &= 0xFFFF00FF;
    writeDword(p_comm, (CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), value | ((unsigned long)byContrast << 8) );

    return TRUE;

}

/******************************************************************************
 *
 * CX2684x_setHue()
 *
 ******************************************************************************/
int
CX2684x_setHue(const CX_COMMUNICATION *p_comm,
            const int hue,
            const unsigned char decoder)
{
    int ret_val = 0;
    int val = 0;
    unsigned char byHue;
    unsigned long value;

    if((hue > VIDEO_PROCAMP_MAX) || (hue < VIDEO_PROCAMP_MIN))
    {
        return FALSE;
    }

    ret_val = mapM(VIDEO_PROCAMP_MIN, VIDEO_PROCAMP_MAX, hue, SIGNED_BYTE_MIN, SIGNED_BYTE_MAX, &val);
    val = convert_to_twos(val, 8);

    byHue = (unsigned char)(val & 0xff);

    readDword(p_comm, (CX2684x_VDECA_CHROMA_CTRL + (0x200 * decoder)), &value);
    value &= 0xFF00FFFF;
    writeDword(p_comm, (CX2684x_VDECA_CHROMA_CTRL + (0x200 * decoder)), value | ((unsigned long)byHue << 16) );

    return TRUE;

}


/******************************************************************************
 *
 * CX2684x_setSaturation()
 *
 ******************************************************************************/
int
CX2684x_setSaturation(const CX_COMMUNICATION *p_comm,
                   const int saturation,
                   const unsigned char decoder)
{
    int ret_val = 0;
    int val = 0;
    unsigned long value;
    unsigned char bySaturation;

    if((saturation > VIDEO_PROCAMP_MAX) || (saturation < VIDEO_PROCAMP_MIN))
    {
        return FALSE;
    }

    ret_val = mapM(VIDEO_PROCAMP_MIN, VIDEO_PROCAMP_MAX, saturation, UNSIGNED_BYTE_MIN, UNSIGNED_BYTE_MAX, &val);

    bySaturation = (unsigned char)(val & 0xff);

    readDword(p_comm, (CX2684x_VDECA_CHROMA_CTRL + (0x200 * decoder)), &value);
    value &= 0xFFFF0000;
    writeDword(p_comm, (CX2684x_VDECA_CHROMA_CTRL + (0x200 * decoder)), value | ((unsigned long)bySaturation << 8) | bySaturation );

    return TRUE;

}


/******************************************************************************
 *
 * CX2684x_SetSharpness()
 *
 ******************************************************************************/
int
CX2684x_setSharpness(const CX_COMMUNICATION *p_comm,
                  const int sharpness,
                  const unsigned char decoder)
{
    int ret_val = 0;
    int val = 0;
    unsigned long value;
    unsigned char bySharpness;
    unsigned char sharp_value=0;

    /* These are in order from least sharp to most sharp */
    unsigned char sharpness_mappings[5] =
    {
        0x00,   /* sharpness enhancement (aka luma peaking) disabled */
        0x04,   /* +2.0dB response @ center freq */
        0x05,   /* +3.5dB response @ center freq */
        0x06,   /* +5.0dB response @ center freq */
        0x07    /* +6.0dB response @ center freq */
    };

    if((sharpness > 100) || (sharpness < VIDEO_PROCAMP_MIN))
    {
        return FALSE;
    }

    ret_val = mapM(VIDEO_PROCAMP_MIN, 100, sharpness, UNSIGNED_BYTE_MIN, 4, &val);

    bySharpness = (unsigned char)(val & 0x7);
    sharp_value |= sharpness_mappings[bySharpness];

    readDword(p_comm, (CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), &value);
    value &= 0xFFF8FFFF;
    writeDword(p_comm, (CX2684x_VDECA_LUMA_CTRL + (0x200 * decoder)), value | ((unsigned long)sharp_value << 16) );

    return TRUE;
}

/*******************************************************************************************************
 *
 * CX2684x_InitVPE
 * Initialize Cx2684x VPE (video packing engine)
 *
 *******************************************************************************************************/
static int
CX2684x_InitVPE(const CX_COMMUNICATION *p_comm,
             const unsigned char strip_invalid_pixels)
{
    int i;
    int result = TRUE;
    unsigned long value = 0;

    for (i = 0; i < 8; i++)
    {
        value = 0x02000000 + 0x200 * i;

        result &= writeDword(p_comm, CX2684x_VP_VPE_A0 + (0x20 * i), value);         /* the source of video packing engine: Video Decoder A Scaled */
        value &= 0xFFFFE0FD;
        value |= ((i * 2) << 8);                                                  /* set vpe src sel */
        value |= ((strip_invalid_pixels & 1) << 1);                               /* set strip invalid pixel */
//        result &= writeDword(p_comm, CX2684x_VP_VPE_0_B + (0x08 * i), 0x00F0D0B0);   /* Default value */
        result &= writeDword(p_comm, CX2684x_VP_VPE_A0 + (0x20 * i), value | 0x01);  /* Enable Video Packing Engine */
    }

    /* VPE8-15's source are Vin1-8, Don't strip invalid pixel */
    for (i=0; i < 8; i++)
    {
        value = 0x02000000 + 0x200 * i;
        result &= writeDword(p_comm, CX2684x_VP_VPE_A8 + (0x20 * i), value);         /* the source of video packing engine: Video Decoder A Scaled */
        value &= 0xFFFFE0FD;
        value |= ((i * 2) << 8);                                                  /* set vpe src sel */
        value |= ((strip_invalid_pixels & 1) << 1);                               /* set strip invalid pixel */
//        result &= writeDword(p_comm, CX2684x_VP_VPE_8_B + (0x08 * i), 0x00F0D0B0);   /* Default value */
        result &= writeDword(p_comm, CX2684x_VP_VPE_A8 + (0x20 * i), value | 0x01);  /* Enable Video Packing Engine */
    }

    return result;
}


/*******************************************************************************************************
 *
 * CX2684x_setupByteInterleaveOutput
 * Setup Cx2684x as byte interleave output port
 *
 *******************************************************************************************************/
int
CX2684x_setupByteInterleaveOutput(const CX_COMMUNICATION *p_comm,
                              const unsigned char num_channels, /* Max 4 channels per port, 1, 2, 4, 8 */
                              const unsigned long size,         /* 960, 720, 360, 352, 320 */
                              const unsigned long output_rate,  /* 13.5 27MHz, 54MHz, 108MHz, 144MHz */
                              const unsigned char dvp_port,     /* port A, B, C, D */
                              unsigned long *src_sel,           /* Video source/s selected 0-7 */
                              const unsigned char strip_invalid_pixels,
                              const unsigned long *standard,
                              unsigned long *video_extend_mode)
{
    unsigned long value;
    int i, j;
    int result = TRUE;
    unsigned long output_mode = OMUXMODE_PASSTHROUGH;
    unsigned long hscale = PIXELS_WIDTH720;
    unsigned long demux_mode = 0;
    unsigned long demux_mode_edge = DVP_DEMUX_MODE_SINGLE_EDGE;

    unsigned long omux_clk_sel = 0;
    unsigned long omux_clk_pol = DVP_OMUX_CLK_POL_INVERTED;

    switch(num_channels)
    {
        case 1:
            demux_mode = DVP_DEMUX_MODE_1STREAM;
            break;
        case 2:
            demux_mode = DVP_DEMUX_MODE_2STREAMS;
            break;
        case 4:
        default:
            demux_mode = DVP_DEMUX_MODE_4STREAMS;
            break;
    }

    switch(output_rate)
    {
        case OUTPUT_RATE_13PT5_MHZ: /* 13.5MHz */
            if (num_channels == 2)
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;

            output_mode = OMUXMODE_1_STREAM_REMUX; /* 1 stream remixed */
            break;

        case OUTPUT_RATE_27_MHZ: /* 27MHz */
            if (num_channels == 2)
                output_mode = OMUXMODE_2_STREAM_REMUX; /* 2 stream remuxed, byte interleaved */

            if (num_channels == 2 || num_channels == 4)
            {
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;
            }
            break;

        case OUTPUT_RATE_54_MHZ: /* 54MHz*/
            if (num_channels == 4)
            {
                output_mode = OMUXMODE_4_STREAM_REMUX; /* 4 stream remuxed, byte interleaved */
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;
            }
            else
                output_mode = OMUXMODE_2_STREAM_REMUX; /* 2 stream remuxed, byte interleaved */

            break;

        case OUTPUT_RATE_108_MHZ: /* 108MHz*/
            if (num_channels == 8)
                output_mode = OMUXMODE_8_STREAM_REMUX; /* 8 stream remuxed, byte interleaved */
            else
                output_mode = OMUXMODE_4_STREAM_REMUX; /* 4 stream remuxed, byte interleaved */
            break;
        case OUTPUT_RATE_144_MHZ: /* 144MHz*/
            if (num_channels == 4)
                output_mode = OMUXMODE_4_STREAM_REMUX; /* 4 stream remuxed, byte interleaved */
            omux_clk_sel = 1;
            break;
        case 0: /* disabled */
        default:
            output_mode = OMUXMODE_1_STREAM_REMUX;
            //output_mode = OMUXMODE_DISABLED; //TO DO: from RDK
            break;
    }

    if ( demux_mode_edge == DVP_DEMUX_MODE_DUAL_EDGE )
    {
        omux_clk_pol = DVP_OMUX_CLK_POL_NONINVERTED;
    }

    if ( output_mode == OMUXMODE_PASSTHROUGH )
    {
        omux_clk_pol = DVP_OMUX_CLK_POL_INVERTED; /*DVP_OMUX_CLK_POL_NONINVERTED;*/
    }

    /* Set Device ID to 0x1 */
    readDword(p_comm, CX2684x_REV_ID, &value);
    value &= 0xF0FFFFFF;
    writeDword(p_comm, CX2684x_REV_ID, value | 0x01000000);

    switch (size)
    {
        case 320:
            hscale = 0x13E34B;
            break;
        case 352:
            hscale = 0x10A273;
            break;
        case 360:
            hscale = PIXELS_WIDTH360;
            break;
        case 176:
            hscale = PIXELS_WIDTH176;
            break;
        case 160:
            hscale = PIXELS_WIDTH160;
            break;
        case 704:
            hscale = PIXELS_WIDTH704;
            break;
        case 960:
            hscale = PIXELS_WIDTH960;
            omux_clk_sel = 1;
        default:
            hscale = PIXELS_WIDTH720;
            break;
    }

    for (i=0; i < 8; i++)
    {
        /* Scale to pixels line length */
        result &= readDword(p_comm, CX2684x_VDECA_HSCALE_CTRL + (0x200 * i), &value);
        value &= 0xFF000000;
        result &= writeDword(p_comm, CX2684x_VDECA_HSCALE_CTRL + (0x200 * i), value | hscale);

        /* Camera ID and Stream ID insertion */
        result &= writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 + (0x200 * i),0x00130521 | 0x80);

        result &= readDword(p_comm, CX2684x_VDECA_VSCALE_CTRL + (0x200 * i), &value);
        value &= 0x01000000;
        result &= writeDword(p_comm, CX2684x_VDECA_VSCALE_CTRL + (0x200 * i), value | 0x00080000);
    }

    /* set selected source */
    if ((num_channels < 8) && (output_mode != OMUXMODE_PASSTHROUGH))
    {
        for (j=0; j < num_channels; j++)
        {
            CX2684x_setVideoPackExtend(p_comm, (dvp_port * 4) + j, video_extend_mode[dvp_port*4 + j]);
            CX2684x_setVideoPackEn(p_comm, (dvp_port * 4) + j, (src_sel[j+dvp_port * 4] * 2),  j % 4, 0x3, strip_invalid_pixels);
            CX2684x_setVideoPackFormat(p_comm, (dvp_port * 4) + j, standard[j]);
        }
    }

    /* pad_pwr_dn = 0 to enable VP input pads */
    result &= readDword(p_comm, CX2684x_HOST_REG1, &value);
    value &= 0xFFFFFFF7;
    result &= writeDword(p_comm, CX2684x_HOST_REG1, value);

    result &= readDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFFFFFFFE;
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), value);   /* Disable dvp_in_en */

    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFFF3FFFE;
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value);   /* Disable omux_dat_oe and omux_clk_oe and omux_remux_en */

    /* Set OMUX registers */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFF8C1F09;  //TO DO: RDK=0xFF9C1F09                                                     /* Also clear omux clock select bit[20] */
    if ( output_mode == OMUXMODE_PASSTHROUGH )
    {
        value &= 0xFFFFE0FF;
        value |= ((src_sel[0] * 2) << 8);
    }

    if ( demux_mode_edge == DVP_DEMUX_MODE_DUAL_EDGE )
    {
        value |= (1 << 21);                                                    /* set omux clk_dly_sel_hi */
        value |= (7 << 13);                                                    /* set omux clk_dly_sel_lo */
        value |= (1 << 7);                                                     /* set omux clk_dly_en */
    }

    value |= (omux_clk_sel << 20);                                             /* set omux clock select */
    value |= (omux_clk_pol << 17);                                             /* set omux clock polarity */
    value |= (demux_mode_edge << 16);                                          /* set omux clock mode */
    value |= (output_mode << 4);                                               /* set omux out mode */
    value |= (DVP_OMUX_STREAM_BW_27MBYTES_1STREAM << 2);                       /* hard-coded for now */
    value |= (DVP_OMUX_PACK_ENG_SEL_LOW << 1);                                 /* hard-coded for now */
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value);

    /* Enable omux_remux_en */
    /* Enable omux_dat_oe and omux_clk_oe */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value | 0xC0001 );

    result &= writeDword(p_comm, CX2684x_VP_VPE_MISC, 0x74d42310);

    /* soft reset for VP */
    result &= readDword(p_comm, CX2684x_CHIP_RST, &value );
    result &= writeDword(p_comm, CX2684x_CHIP_RST, value | 0x4 );

    return result;
}


/*******************************************************************************************************
 *
 * CX2684x_setupByteInterleaveInput
 * setup Cx2684x as input port
 *
 *******************************************************************************************************/
int
CX2684x_setupByteInterleaveInput(const CX_COMMUNICATION *p_comm,    //TO DO: different from RDK
                              const unsigned char num_channels, /* Max 4 channels per port, 1, 2, 4 */
                              const unsigned long size,         /* 720, 352, 320 */
                              const unsigned long output_rate,  /* 13.5MHz, 27MHz, 54MHz, 108MHz */
                              const unsigned char dvp_port,     /* port A, B, C, D */
                              unsigned long *src_sel)
{
    int result = TRUE;
    int i;
    unsigned long value = 0;
    unsigned char strip_invalid_pixels = 0;

    unsigned long demux_mode = DVP_DEMUX_MODE_1STREAM;
    unsigned long demux_mode_edge = DVP_DEMUX_MODE_SINGLE_EDGE;

    unsigned long stream_id = 0;

    switch (num_channels)
    {
        case 1:
            demux_mode = DVP_DEMUX_MODE_1STREAM;
            break;
        case 2:
            demux_mode = DVP_DEMUX_MODE_2STREAMS;
            break;
        case 4:
            demux_mode = DVP_DEMUX_MODE_4STREAMS;
            break;
    }

    switch (output_rate)
    {
        case OUTPUT_RATE_13PT5_MHZ:  /* 13.5 MHz */
            if (num_channels == 2)
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;
            break;
        case OUTPUT_RATE_27_MHZ:    /* 27 MHz */
            if (num_channels == 2 || num_channels == 4)
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;
            break;
        case OUTPUT_RATE_54_MHZ:    /* 54 MHz */
            if (num_channels == 4)
                demux_mode_edge = DVP_DEMUX_MODE_DUAL_EDGE;
            break;
        case OUTPUT_RATE_108_MHZ:   /* 108 MHz */
        default:
            demux_mode_edge = DVP_DEMUX_MODE_SINGLE_EDGE;
            break;
    }

    switch (size)
    {
        case 320:
        case 352:
            strip_invalid_pixels = 1;
            break;
    }

    for (i=0; i < num_channels; i++)
    {
        /* Camera ID and Stream ID insertion */
        result &= writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1 + (0x200 * (src_sel[0]+i)),0x00130521 | 0x80);
    }

    for (i = 0; i < num_channels; i++)
    {
        result &= readDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), &value );
        value &= 0x0000ffff;
        stream_id |= (src_sel[i] << (16 + i*4));
        result &= writeDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), value );
    }

    result &= CX2684x_InitVPE(p_comm, strip_invalid_pixels);

    result &= readDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFFFFFFFE;
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), value);   /* Disable dvp_in_en */

    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFFF3FFFE;
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value);   /* Disable omux_dat_oe and omux_clk_oe and omux_remux_en */

    /* Set DEMUX registers */
    result &= readDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), &value );
    value &= 0x0000FFC9;                                                       /* Set stream ids to 0 */
    value |= (demux_mode << 4);                                                /* Set demux mode, hard-coded */
    value |= (demux_mode_edge << 2);                                           /* Set clock mode */
    value |= (DVP_DEMUX_CLK_POL_NONINVERTED << 1);                             /* Set clock polarity */
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), value);

    /* Set OMUX registers */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    value &= 0xFFF0FF88;
    value |= (DVP_OMUX_CLK_POL_NONINVERTED << 17);                             /* set omux clock polarity */
    value |= (DVP_OMUX_CLK_MODE_DUAL_EDGE << 16);                              /* set omux clock mode, hard-coded for now */
    value |= (OMUXMODE_4_STREAM_REMUX << 4);                                   /* set omux out mode, hard-coded for now */
    value |= (DVP_OMUX_STREAM_BW_27MBYTES_1STREAM << 2);                       /* hard-coded for now */
    value |= (DVP_OMUX_PACK_ENG_SEL_HIGH << 1);                                /* hard-coded for now */
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value);

    result &= readDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), &value );
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A + (0x04 * dvp_port), value | 1); /* Enable dvp_in_en */

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setMacrovisionEnable
 *
 *******************************************************************************************************/
int
CX2684x_setMacrovisionEnable(const CX_COMMUNICATION *p_comm,
                          const unsigned char enable)
{
    int result = TRUE;
    unsigned long value = 0;
    int i;
    int _num_decoders = MAX_DECODERS;

    for (i=0; i < _num_decoders; i++)
    {
        result &= readDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*i), &value);
        if (enable)
        {
            value |= 0x40000000; /* set bit 30, vga_crush_en */
        }
        else
        {
            value &= 0xBFFFFFFF;
        }

        result &= writeDword(p_comm, CX2684x_VDECA_DFE_CTRL1+(0x200*i), value);
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setUvSuppress
 *
 *******************************************************************************************************/
int
CX2684x_setUvSuppress(const CX_COMMUNICATION *p_comm,
                   const unsigned char value)
{
    int result = TRUE;
    unsigned long temp = 0;
    int i;
    int _num_decoders = MAX_DECODERS;

    for (i=0; i < _num_decoders; i++)
    {
        result &= readDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1+(0x200*i), &temp);
        temp &= 0xFFF9FFFF;
        temp |= ((value & 3) << 17);
        result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1+(0x200*i), temp);
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoPackEn
 *
 *******************************************************************************************************/
static int CX2684x_setVideoPackEn(const CX_COMMUNICATION *p_comm,
                            const unsigned long vpe_sel,
                            const unsigned long src_sel,
                            const unsigned long vpe_id,
                            const unsigned long id_len,
                            const unsigned long strip_en)
{
    int result = TRUE;
    unsigned long value;
    unsigned long port_offset = 0x20;

    result &= readDword(p_comm, CX2684x_VP_VPE_A0 + (port_offset * vpe_sel), &value);
    value &= 0xfffff000;
    value |= (src_sel << 8);
    value |= (vpe_id << 4);
    value |= (id_len << 2);
    value |= (strip_en << 1);
    value |= 0x1; /* enable vpe function */
    result &= writeDword(p_comm, CX2684x_VP_VPE_A0 + (port_offset * vpe_sel), value);
    return result;
}

/******************************************************************************
 *
 * CX2684x_setVideoPackDisable()
 *
 ******************************************************************************/
int
CX2684x_setVideoPackDisable(const CX_COMMUNICATION *p_comm,
                         const unsigned char channel,      /* specific channel */
                         const unsigned char dvp_port)     /* port A, B, C, D */
{
    int result = TRUE;
    int vpe_sel = 0;
    unsigned long value;

    if ( channel < 8 )
    {
        vpe_sel = dvp_port*4 + channel;
        result &= readDword(p_comm, CX2684x_VP_VPE_A0 + (0x20 * vpe_sel), &value);
        value &= 0xfffffffe;    /* disable VPE function */
        result &= writeDword(p_comm, CX2684x_VP_VPE_A0 + (0x20 * vpe_sel), value);
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setup16CifBigFrame
 * Setup Cx2684x as 16 cif big frame interleave
 *
 *******************************************************************************************************/
int
CX2684x_setup16CifBigFrame(const CX_COMMUNICATION *p_comm, unsigned long standard)
{
    int i, result = TRUE;
    unsigned long value;

    /* Set OMUX registers */
    /* Port B */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * 1), &value );
    value |= (DVP_OMUX_CLK_POL_INVERTED << 17);                             /* set omux clock polarity */
    value |= (DVP_OMUX_STREAM_BW_13PT5MBYTES_1STREAM << 2);
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * 1), value);

    /* Port C */
    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * 2), &value );
    value |= (DVP_OMUX_CLK_POL_INVERTED << 17);                             /* set omux clock polarity */
    value |= (DVP_OMUX_STREAM_BW_13PT5MBYTES_1STREAM << 2);
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * 2), value);

    result &= readDword(p_comm, CX2684x_VP_OMUX_A, &value);
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A,  value & 0xfffffffe); /* disable remux function on PA */

    result &= readDword(p_comm, CX2684x_VP_DEMUX_A, &value);
    value &= 0x0000ffcc;            /* set to Single Stream input */
    value |= 0x32100000;
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A, value);
    result &= writeDword(p_comm, CX2684x_VP_DEMUX_A, value | 1); /* enable port */


    result &= readDword(p_comm, CX2684x_HOST_REG1, &value);
    value &= 0xff00;
    value |= 0x44; /* power on pad */
    result &= writeDword(p_comm, CX2684x_HOST_REG1, value);


    result &= readDword(p_comm, CX2684x_VP_OMUX_A, &value);
    value &= 0xffff1ff7;
    value  |= (1 << 7);
    value |= (2 << 13);
    result &= writeDword(p_comm, CX2684x_VP_OMUX_A, value);

    result &= readDword(p_comm, CX2684x_VP_OMUX_E, &value);
    value &= 0xfff1ffff;
    value |= (1 << 17); /* clock polarity */
    value |= (1 << 18); /* enable clk */
    value |= (1 << 19); /* enable data */
    value |= 0x01;      /* enable */
    result &= writeDword(p_comm, CX2684x_VP_OMUX_E, value);

    for (i=0; i < MAX_DECODERS; i++)
    {
        result &= readDword(p_comm, CX2684x_VDECA_OUT_CTRL1+(0x200*i), &value);
        writeDword(p_comm, CX2684x_VDECA_OUT_CTRL1+(0x200*i), value & 0xFFFFFFEF);
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoPackExtend
 *
 *******************************************************************************************************/
static int CX2684x_setVideoPackExtend(const CX_COMMUNICATION *p_comm,
                                   const unsigned long vpe_sel,
                                   const unsigned long video_extend_mode)
{
    int result = TRUE;
    unsigned long value;
    unsigned long port_offset = 0x20;
    unsigned long hact_len;
    unsigned long invld_pix_strp_en;
    unsigned long hpre_en;
    unsigned long hpst_en;
    unsigned long hpr_padding_len;
    unsigned long hpst_padding_len;
    unsigned long ln_vld_thrshld;

    switch (video_extend_mode)
    {
        case VIDEO_EXTEND_D1: /* D1 to D1 */
            hact_len = 720;
            invld_pix_strp_en  = 0;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_960H: /* 960 to 960 */
            hact_len = 960;
            invld_pix_strp_en  = 0;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_960H_D1: /* 960 cropped to D1 */
            hact_len = 720;
            invld_pix_strp_en  = 0;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 64;
            break;
        case VIDEO_EXTEND_D1_HBI_960H: /* D1 HBI extend to 960 */
            hact_len = 720;
            invld_pix_strp_en  = 0;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 448;
            break;
        case VIDEO_EXTEND_D1_ACT_960H: /* D1 hactive extend to 960 */
            hact_len = 720;
            invld_pix_strp_en  = 0;
            hpre_en  = 1;
            hpst_en  = 1;
            hpr_padding_len  = 120;
            hpst_padding_len  = 120;
            ln_vld_thrshld  = 288;
            break;
        case VIDEO_EXTEND_CIF: /* H D1 to H d1 */
            hact_len = 352;
            invld_pix_strp_en  = 1;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_HLF_960H: /* H E1 to H E1 */
            hact_len = 480;
            invld_pix_strp_en  = 1;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_HLF_960H_HBI: /* H E1 to E1 */
            hact_len = 480;
            invld_pix_strp_en  = 1;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 496;
            break;
        case VIDEO_EXTEND_HLF_960H_HLF_D1: /* H E1 cropped to H D1 */
            hact_len = 360;
            invld_pix_strp_en  = 1;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_HLF_D1_HBI_960H: /* H D1 HBI extend to E1 */
            hact_len = 360;
            invld_pix_strp_en  = 1;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
        default:
            hact_len = 720;
            invld_pix_strp_en  = 0;
            hpre_en  = 0;
            hpst_en  = 0;
            hpr_padding_len  = 0;
            hpst_padding_len  = 0;
            ln_vld_thrshld  = 256;
            break;
    }

    result &= readDword(p_comm, CX2684x_VP_VPE_E0 + (port_offset * vpe_sel), &value);
    value  &= 0xFC000000;
    value  |= hpr_padding_len;
    value  |= (hpst_padding_len << 8);
    value  |= (hact_len << 16);
    result &= writeDword(p_comm, CX2684x_VP_VPE_E0 + (port_offset * vpe_sel), value);

    result &= readDword(p_comm, CX2684x_VP_VPE_A0 + (port_offset * vpe_sel), &value);
    value  &= 0xFFFF3F0D;
    value  |= (invld_pix_strp_en << 1);
    value  |= (hpre_en << 14);
    value  |= (hpst_en << 15);
    result &= writeDword(p_comm, CX2684x_VP_VPE_A0 + (port_offset * vpe_sel), value);

    result &= readDword(p_comm, CX2684x_VP_VPE_F0 + (port_offset * vpe_sel), &value);
    value  &= 0xFFFFFE00;
    value  |= ln_vld_thrshld;
    result &= writeDword(p_comm, CX2684x_VP_VPE_F0 + (port_offset * vpe_sel), value);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoPackFormat
 *
 *******************************************************************************************************/
static int CX2684x_setVideoPackFormat(const CX_COMMUNICATION *p_comm,
                                   const unsigned long vpe_sel,
                                   const unsigned long standard)
{
    int result = TRUE;
    unsigned long port_offset = 0x20;
    unsigned long value;

    result &= readDword(p_comm,  CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (0x14 << 11);
    value  |= (0x10 << 27);
    result &= writeDword(p_comm, CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (0x14 << 11);
    value  |= (0x1C << 27);
    result &= writeDword(p_comm, CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (0x18 << 11);
    value  |= (0x1C << 27);
    result &= writeDword(p_comm, CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), value);

    if ( standard == NTSC_VIDEO_STD || standard == NTSC_EFFIO_STD )
    {
        result &= readDword(p_comm,  CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 19;
        value  |= (259 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), value);

        result &= readDword(p_comm,  CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 262;
        value  |= (282 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), value);

        result &= readDword(p_comm,  CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 522;
        value  |= (525 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), value);
    }
    else
    {
        result &= readDword(p_comm,  CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 22;
        value  |= (310 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_B0 + (port_offset * vpe_sel), value);

        result &= readDword(p_comm,  CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 312;
        value  |= (335 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_C0 + (port_offset * vpe_sel), value);

        result &= readDword(p_comm,  CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), &value);
        value  &= 0xFC00FC00;
        value  |= 623;
        value  |= (625 << 16);
        result &= writeDword(p_comm, CX2684x_VP_VPE_D0 + (port_offset * vpe_sel), value);
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoOMUX
 *
 *******************************************************************************************************/
static int CX2684x_setVideoOMUX(const CX_COMMUNICATION *p_comm,
                             const unsigned char dvp_port,     /* port A, B, C, D */
                             unsigned long output_mode)
{
    int result = TRUE;
    unsigned long value;

    unsigned long omux_clk_sel = 0x0;
    unsigned long omux_clk_pol = DVP_OMUX_CLK_POL_INVERTED;
    unsigned long omux_clk_dly_en = 0;

    unsigned long omux_clk_dly_stage  = 0;
    unsigned long omux_clk_dly_sel_lo =  omux_clk_dly_stage & 0x7;
    unsigned long omux_clk_dly_sel_hi = (omux_clk_dly_stage & 0x8) >> 3;

    result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), &value );
    value  &= 0xFFC11F08;
    value  |= (output_mode << 4);                           /* set omux out mode */
    value  |= (omux_clk_pol << 17);                         /* set omux clock polarity */
    value  |= (omux_clk_dly_sel_lo << 13);                  /* set omux clock delay stage select low for clock output */
    value  |= (omux_clk_dly_sel_hi << 21);                  /* set omux clock delay stage select high for clock output */
    value  |= (omux_clk_dly_en << 7);                       /* enable/disable delay chain for clock output */
    value  |= (1 << 19);                                    /* set omux data output */
    value  |= (1 << 18);                                    /* set omux clock output */
    value  |= (omux_clk_sel << 20);                         /* set omux clock select: 0x0 D1, 0x1 960H */

    value  |= (DVP_OMUX_STREAM_BW_27MBYTES_1STREAM << 2);   /* set bandwidth of each stream */
    value  |= (DVP_OMUX_PACK_ENG_SEL_LOW << 1);             /* set packing engine group for remux */
    value  |= 0x1;                                          /* enable the remux function */

    result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * dvp_port), value);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoFrmrFormat
 *
 * Set Video Framer Format
 *
 * Format the video output according to frm mode
 * video_frmr_mode:
 *        0: D1 NTSC Normal Frame{I}
 *        1: D1 PAL Normal Frame{I}
 *        2: E1 NTSC Normal Frame{I}
 *        3: E1 PAL Normal Frame{I}
 *        4: D1 NTSC Normal Frame{II}
 *        5: D1 PAL Normal Frame{II}
 *        6: E1 NTSC Normal Frame{II}
 *        7: E1 PAL Normal Frame{II}
 *        8: D1 NTSC Big Frame x2way
 *        9: D1 PAL Big Frame x2way
 *        A: E1 NTSC Big Frame x2way
 *        B: E1 PAL Big Frame x2way
 *        C: D1 NTSC Big Frame x4way
 *        D: D1 PAL Big Frame x4way
 *        E: E1 NTSC Big Frame x4way
 *        F: E1 PAL Big Frame x4way
 * frmr_sel:
 *        0x0: framer 0;
 *        0x1: framer 1;
 *        0x2: framer 2;
 *        0x3: framer 3;
 *
 *******************************************************************************************************/
static int CX2684x_setVideoFrmrFormat(const CX_COMMUNICATION *p_comm,
                                   const unsigned long frmr_sel,
                                   const unsigned long frmr_mode)
{
    int i;
    int result = TRUE;
    unsigned long value;
    unsigned long port_offset = 0x20;

    unsigned long fld0_cnt_0  = 1;
    unsigned long fld0_cnt_1  = 2;
    unsigned long fld0_cnt_2  = 242;
    unsigned long fld1_cnt_0  = 264;
    unsigned long fld1_cnt_1  = 265;
    unsigned long fld1_cnt_2  = 505;
    unsigned long sync_mode   = 9;
    unsigned long sync_ln_dly = 17;

    switch (frmr_mode)
    {
        case VIDEO_D1_NTSC_NORMAL_FRAMER_I: /* D1 NTSC Normal Frame{I} */
        case VIDEO_E1_NTSC_NORMAL_FRAMER_I: /* E1 NTSC Normal Frame{I} */
            break;
        case VIDEO_D1_PAL_NORMAL_FRAMER_I: /* D1 PAL Normal Frame{I} */
        case VIDEO_E1_PAL_NORMAL_FRAMER_I: /* E1 PAL Normal Frame{I} */
            fld0_cnt_2  = 290;
            fld1_cnt_0  = 314;
            fld1_cnt_1  = 315;
            fld1_cnt_2  = 603;
            sync_ln_dly = 20;
            break;
        case VIDEO_D1_NTSC_NORMAL_FRAMER_II: /* D1 NTSC Normal Frame{II} */
        case VIDEO_E1_NTSC_NORMAL_FRAMER_II: /* E1 NTSC Normal Frame{II} */
            fld0_cnt_0  = 3;
            fld0_cnt_1  = 22;
            fld0_cnt_2  = 262;
            fld1_cnt_0  = 265;
            fld1_cnt_1  = 285;
            fld1_cnt_2  = 525;
            sync_mode   = 10;
            sync_ln_dly = 0;
            break;
        case VIDEO_D1_PAL_NORMAL_FRAMER_II: /* D1 PAL Normal Frame{II} */
        case VIDEO_E1_PAL_NORMAL_FRAMER_II: /* E1 PAL Normal Frame{II} */
            fld0_cnt_0  = 22;
            fld0_cnt_1  = 310;
            fld0_cnt_2  = 312;
            fld1_cnt_0  = 335;
            fld1_cnt_1  = 623;
            fld1_cnt_2  = 625;
            sync_ln_dly = 0;
            break;
        case VIDEO_D1_NTSC_BIG_FRAMER_X2WAY: /* D1 NTSC Big Frame x2way */
        case VIDEO_E1_NTSC_BIG_FRAMER_X2WAY: /* E1 NTSC Big Frame x2way */
            break;
        case VIDEO_D1_PAL_BIG_FRAMER_X2WAY:  /* D1 PAL Big Frame x2way */
        case VIDEO_E1_PAL_BIG_FRAMER_X2WAY:  /* E1 PAL Big Frame x2way */
            fld0_cnt_2  = 290;
            fld1_cnt_0  = 314;
            fld1_cnt_1  = 315;
            fld1_cnt_2  = 603;
            sync_ln_dly = 20;
            break;
        case VIDEO_D1_NTSC_BIG_FRAMER_X4WAY: /* D1 NTSC Big Frame x4way */
        case VIDEO_E1_NTSC_BIG_FRAMER_X4WAY: /* E1 NTSC Big Frame x4way */
            break;
        case VIDEO_D1_PAL_BIG_FRAMER_X4WAY:  /* D1 PAL Big Frame x4way */
        case VIDEO_E1_PAL_BIG_FRAMER_X4WAY:  /* E1 PAL Big Frame x4way */
            fld0_cnt_2  = 290;
            fld1_cnt_0  = 314;
            fld1_cnt_1  = 315;
            fld1_cnt_2  = 603;
            sync_ln_dly = 20;
            break;
        default:
            break;
    }

    for (i = 0; i < 4; i++)
    {
        result &= readDword(p_comm,  CX2684x_VP_FRMR0_B0 + (port_offset * frmr_sel) + (i*0x20), &value);
        value  &= 0xFC00FC00;
        value  |= fld0_cnt_0;
        value  |= (fld0_cnt_1 << 16);
        result &= writeDword(p_comm, CX2684x_VP_FRMR0_B0 + (port_offset * frmr_sel) + (i*0x20), value);


        result &= readDword(p_comm,  CX2684x_VP_FRMR0_C0 + (port_offset * frmr_sel) + (i*0x20), &value);
        value  &= 0xFC00FC00;
        value  |= fld0_cnt_2;
        value  |= (fld1_cnt_0 << 16);
        result &= writeDword(p_comm, CX2684x_VP_FRMR0_C0 + (port_offset * frmr_sel) + (i*0x20), value);

        result &= readDword(p_comm,  CX2684x_VP_FRMR0_D0 + (port_offset * frmr_sel) + (i*0x20), &value);
        value  &= 0xFC00FC00;
        value  |= fld1_cnt_1;
        value  |= (fld1_cnt_2 << 16);
        result &= writeDword(p_comm, CX2684x_VP_FRMR0_D0 + (port_offset * frmr_sel) + (i*0x20), value);
    }

    result &= readDword(p_comm,  CX2684x_VP_VPE_A8 + (port_offset * frmr_sel), &value);
    value  &= 0xFE00FFFF;
    value  |= (sync_ln_dly << 20);
    value  |= (sync_mode << 16);
    result &= writeDword(p_comm, CX2684x_VP_VPE_A8 + (port_offset * frmr_sel), value);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoFrmrFormatTFVB
 *
 * Set Video Framer Format TFVB
 *
 * Format the video output according to frm mode
 * video_frmr_mode:
 *        0: D1 NTSC Normal Frame{I}
 *        1: D1 PAL Normal Frame{I}
 *        2: E1 NTSC Normal Frame{I}
 *        3: E1 PAL Normal Frame{I}
 *        4: D1 NTSC Normal Frame{II}
 *        5: D1 PAL Normal Frame{II}
 *        6: E1 NTSC Normal Frame{II}
 *        7: E1 PAL Normal Frame{II}
 *        8: D1 NTSC Big Frame x2way
 *        9: D1 PAL Big Frame x2way
 *        A: E1 NTSC Big Frame x2way
 *        B: E1 PAL Big Frame x2way
 *        C: D1 NTSC Big Frame x4way
 *        D: D1 PAL Big Frame x4way
 *        E: E1 NTSC Big Frame x4way
 *        F: E1 PAL Big Frame x4way
 * frmr_sel:
 *        0x0: framer 0;
 *        0x1: framer 1;
 *        0x2: framer 2;
 *        0x3: framer 3;
 *
 *******************************************************************************************************/
static int CX2684x_setVideoFrmrFormatTFVB(const CX_COMMUNICATION *p_comm,
                                       const unsigned long frmr_sel,
                                       const unsigned long frmr_mode)
{
    int result = TRUE;
    unsigned long value;
    unsigned long port_offset = 0x20;

    unsigned long ch0_fld0_cnt_0_tfvb   = 0x1c;
    unsigned long ch0_fld0_cnt_1_tfvb   = 0x14;
    unsigned long ch0_fld0_cnt_2_tfvb   = 0x10;
    unsigned long ch0_fld1_cnt_0_tfvb   = 0x15;
    unsigned long ch0_fld1_cnt_1_tfvb   = 0x1c;
    unsigned long ch0_fld1_cnt_2_tfvb   = 0x18;
    unsigned long ch1_fld0_cnt_0_tfvb   = 0x1c;
    unsigned long ch1_fld0_cnt_1_tfvb   = 0x14;
    unsigned long ch1_fld0_cnt_2_tfvb   = 0x10;
    unsigned long ch1_fld1_cnt_0_tfvb   = 0x15;
    unsigned long ch1_fld1_cnt_1_tfvb   = 0x1c;
    unsigned long ch1_fld1_cnt_2_tfvb   = 0x18;
    unsigned long ch2_fld0_cnt_0_tfvb   = 0x1c;
    unsigned long ch2_fld0_cnt_1_tfvb   = 0x14;
    unsigned long ch2_fld0_cnt_2_tfvb   = 0x10;
    unsigned long ch2_fld1_cnt_0_tfvb   = 0x15;
    unsigned long ch2_fld1_cnt_1_tfvb   = 0x1c;
    unsigned long ch2_fld1_cnt_2_tfvb   = 0x18;
    unsigned long ch3_fld0_cnt_0_tfvb   = 0x1c;
    unsigned long ch3_fld0_cnt_1_tfvb   = 0x14;
    unsigned long ch3_fld0_cnt_2_tfvb   = 0x10;
    unsigned long ch3_fld1_cnt_0_tfvb   = 0x15;
    unsigned long ch3_fld1_cnt_1_tfvb   = 0x1c;
    unsigned long ch3_fld1_cnt_2_tfvb   = 0x18;

    switch (frmr_mode)
    {
        case VIDEO_D1_NTSC_NORMAL_FRAMER_II:
        case VIDEO_E1_NTSC_NORMAL_FRAMER_II:
            ch0_fld1_cnt_0_tfvb =0x14;
            ch1_fld1_cnt_0_tfvb =0x14;
            ch2_fld1_cnt_0_tfvb =0x14;
            ch3_fld1_cnt_0_tfvb =0x14;
            break;
        case VIDEO_D1_PAL_NORMAL_FRAMER_II:
        case VIDEO_E1_PAL_NORMAL_FRAMER_II:
            ch0_fld0_cnt_0_tfvb = 0x14;
            ch0_fld0_cnt_1_tfvb = 0x10;
            ch0_fld0_cnt_2_tfvb = 0x14;
            ch0_fld1_cnt_0_tfvb = 0x1c;
            ch0_fld1_cnt_1_tfvb = 0x18;
            ch0_fld1_cnt_2_tfvb = 0x1c;
            ch1_fld0_cnt_0_tfvb = 0x14;
            ch1_fld0_cnt_1_tfvb = 0x10;
            ch1_fld0_cnt_2_tfvb = 0x14;
            ch1_fld1_cnt_0_tfvb = 0x1c;
            ch1_fld1_cnt_1_tfvb = 0x18;
            ch1_fld1_cnt_2_tfvb = 0x1c;
            ch2_fld0_cnt_0_tfvb = 0x14;
            ch2_fld0_cnt_1_tfvb = 0x10;
            ch2_fld0_cnt_2_tfvb = 0x14;
            ch2_fld1_cnt_0_tfvb = 0x1c;
            ch2_fld1_cnt_1_tfvb = 0x18;
            ch2_fld1_cnt_2_tfvb = 0x1c;
            ch3_fld0_cnt_0_tfvb = 0x14;
            ch3_fld0_cnt_1_tfvb = 0x10;
            ch3_fld0_cnt_2_tfvb = 0x14;
            ch3_fld1_cnt_0_tfvb = 0x1c;
            ch3_fld1_cnt_1_tfvb = 0x18;
            ch3_fld1_cnt_2_tfvb = 0x1c;
            break;
        case VIDEO_D1_NTSC_BIG_FRAMER_X2WAY:
        case VIDEO_D1_PAL_BIG_FRAMER_X2WAY:
        case VIDEO_E1_NTSC_BIG_FRAMER_X2WAY:
        case VIDEO_E1_PAL_BIG_FRAMER_X2WAY:
            ch0_fld1_cnt_0_tfvb = 0x13;
            ch0_fld1_cnt_1_tfvb = 0x13;
            ch0_fld1_cnt_2_tfvb = 0x10;
            ch1_fld0_cnt_0_tfvb = 0x14;
            ch1_fld0_cnt_1_tfvb = 0x1c;
            ch1_fld0_cnt_2_tfvb = 0x18;
            ch1_fld1_cnt_0_tfvb = 0x1b;
            ch1_fld1_cnt_1_tfvb = 0x1b;
            break;
        case VIDEO_D1_NTSC_BIG_FRAMER_X4WAY:
        case VIDEO_D1_PAL_BIG_FRAMER_X4WAY:
        case VIDEO_E1_NTSC_BIG_FRAMER_X4WAY:
        case VIDEO_E1_PAL_BIG_FRAMER_X4WAY:
            ch0_fld1_cnt_0_tfvb = 0x13;
            ch0_fld1_cnt_1_tfvb = 0x13;
            ch0_fld1_cnt_2_tfvb = 0x10;
            ch1_fld0_cnt_0_tfvb = 0x13;
            ch1_fld0_cnt_1_tfvb = 0x13;
            ch1_fld0_cnt_2_tfvb = 0x10;
            ch1_fld1_cnt_0_tfvb = 0x13;
            ch1_fld1_cnt_1_tfvb = 0x13;
            ch1_fld1_cnt_2_tfvb = 0x10;
            ch2_fld0_cnt_0_tfvb = 0x14;
            ch2_fld0_cnt_1_tfvb = 0x1c;
            ch2_fld0_cnt_2_tfvb = 0x18;
            ch2_fld1_cnt_0_tfvb = 0x1b;
            ch2_fld1_cnt_1_tfvb = 0x1b;
            ch2_fld1_cnt_2_tfvb = 0x18;
            ch3_fld0_cnt_0_tfvb = 0x1b;
            ch3_fld0_cnt_1_tfvb = 0x1b;
            ch3_fld0_cnt_2_tfvb = 0x18;
            ch3_fld1_cnt_0_tfvb = 0x1b;
            ch3_fld1_cnt_1_tfvb = 0x1b;
            ch3_fld1_cnt_2_tfvb = 0x18;
            break;
        default:
            break;
    }
    /* Channel 0 */
    result &= readDword(p_comm,  CX2684x_VP_FRMR0_B0 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch0_fld0_cnt_0_tfvb << 11);
    value  |= (ch0_fld0_cnt_1_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_B0 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_C0 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch0_fld0_cnt_2_tfvb << 11);
    value  |= (ch0_fld1_cnt_0_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_C0 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_D0 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch0_fld1_cnt_1_tfvb << 11);
    value  |= (ch0_fld1_cnt_2_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_D0 + (port_offset * frmr_sel), value);

    /* Channel 1 */
    result &= readDword(p_comm,  CX2684x_VP_FRMR0_B1 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch1_fld0_cnt_0_tfvb << 11);
    value  |= (ch1_fld0_cnt_1_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_B1 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_C1 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch1_fld0_cnt_2_tfvb << 11);
    value  |= (ch1_fld1_cnt_0_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_C1 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_D1 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch1_fld1_cnt_1_tfvb << 11);
    value  |= (ch1_fld1_cnt_2_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_D1 + (port_offset * frmr_sel), value);

    /* Channel 2 */
    result &= readDword(p_comm,  CX2684x_VP_FRMR0_B2 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch2_fld0_cnt_0_tfvb << 11);
    value  |= (ch2_fld0_cnt_1_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_B2 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_C2 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch2_fld0_cnt_2_tfvb << 11);
    value  |= (ch2_fld1_cnt_0_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_C2 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_D2 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch2_fld1_cnt_1_tfvb << 11);
    value  |= (ch2_fld1_cnt_2_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_D2 + (port_offset * frmr_sel), value);

    /* Channel 3 */
    result &= readDword(p_comm,  CX2684x_VP_FRMR0_B3 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch3_fld0_cnt_0_tfvb << 11);
    value  |= (ch3_fld0_cnt_1_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_B3 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_C3 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch3_fld0_cnt_2_tfvb << 11);
    value  |= (ch3_fld1_cnt_0_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_C3 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_D3 + (port_offset * frmr_sel), &value);
    value  &= 0x07FF07FF;
    value  |= (ch3_fld1_cnt_1_tfvb << 11);
    value  |= (ch3_fld1_cnt_2_tfvb << 27);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_D3 + (port_offset * frmr_sel), value);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setVideoFrmrMode
 *
 *******************************************************************************************************/
static int CX2684x_setVideoFrmrMode(const CX_COMMUNICATION *p_comm,
                                 const unsigned long frmr_sel,
                                 const unsigned long ch_sel,
                                 const unsigned long video_extend_mode,
                                 const unsigned long src_sel,
                                 const unsigned long id)
{
    int result = TRUE;
    unsigned long value;

    unsigned long frmr_offset = 0x80;
    unsigned long ch_offset   = 0x20;
    unsigned long hact_len;
    unsigned long hpre_en;
    unsigned long hpst_en;
    unsigned long ln_vld_thrshld;

    switch (video_extend_mode)
    {
        case VIDEO_EXTEND_D1:          /* D1 to D1 */
            hact_len = 720;
            hpre_en  = 0;
            hpst_en  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_960H:        /* 960 to 960 */
            hact_len = 960;
            hpre_en  = 0;
            hpst_en  = 0;
            ln_vld_thrshld  = 256;
            break;
        case VIDEO_EXTEND_960H_D1:     /* 960 cropped to D1 */
            hact_len = 720;
            hpre_en  = 0;
            hpst_en  = 0;
            ln_vld_thrshld  = 64;
            break;
        case VIDEO_EXTEND_D1_HBI_960H: /* D1 HBI extend to 960 */
            hact_len = 720;
            hpre_en  = 0;
            hpst_en  = 0;
            ln_vld_thrshld  = 448;
            break;
        case VIDEO_EXTEND_D1_ACT_960H: /* D1 hactive extend to 960 */
            hact_len = 720;
            hpre_en  = 1;
            hpst_en  = 1;
            ln_vld_thrshld  = 288;
            break;
        default:
            hact_len = 720;
            hpre_en  = 0;
            hpst_en  = 0;
            ln_vld_thrshld  = 256;
            break;
    }

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_E0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), &value);
    value  &= 0xFC00FFFF;
    value  |= (hact_len << 16);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_E0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_F0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), &value);
    value  &= 0xFFFFFE00;
    value  |= ln_vld_thrshld;
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_F0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR0_A0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), &value);
    value  &= 0xFFFF200F;
    value  |= (hpst_en << 14);
    value  |= (hpre_en << 15);
    value  |= (src_sel << 8);
    value  |= (id << 4);
    result &= writeDword(p_comm, CX2684x_VP_FRMR0_A0 + (frmr_offset * frmr_sel) + (ch_offset * ch_sel), value);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_enableVideoFramer
 *
 *--- enable framer in Cx26868 to specified mode
 *--- fmr_Sel:
           0--framer0
           1--framer1
           2--framer2
           3--framer3
 *--- vpe_attach:
       0x0~0xf : VPE0~VPE15
 *--- fmr_schm: This field defines the working mode of framer.
    0x0: Fixed order round robin rotating
    0x1: Fixed order time slot round robin rotating
    0x2: Dynamic order round robin rotating
    0x3: Programmable order time slot rotating
 *--- fmr_caps:
        This field defines the ways of framer for "Fixed order round robin rotating",
                                                   Fixed order time slot round robin rotating" and
                                                   Dynamic order round robin rotating".
        Please note that "Programmable order time slot rotating" does not use this parameter,
        but uses "frmr_slot_queue_len" instead.
 *--- fmr_que_len
    This field indicates the queue length for "Programmable order time slot rotating".
        0x0: length is 1.
        0x1: length is 2.
        ...
        ...
        ...
        0xE: length is 15.
        0xF: length is 16.
 *--- fmr_slot_len
    define the time slot length. Unit: microsecond
 *--- fmr_slot_thr
    Start of Frame detection window width inside the time slot. Unit: microsecond
 *--- fmr_slot_que
 * This field defines the channel sequence queue for "Programmable order time slot rotating"
 * Every 2bits stands for 1 channel, which means 16 items at most.
 *
 *******************************************************************************************************/
static int CX2684x_enableVideoFramer(const CX_COMMUNICATION *p_comm,
                                  const unsigned long frmr_sel,
                                  const unsigned long vpe_sel,
                                  const unsigned long frmr_caps,
                                  const unsigned long frmr_schm,
                                  const unsigned long frmr_que_len,
                                  const unsigned long frmr_slot_que,
                                  const unsigned long frmr_slot_len,
                                  const unsigned long frmr_slot_thr)
{
    int result = TRUE;
    unsigned long value;
    unsigned long port_offset = 0x10;

    result &= readDword(p_comm,  CX2684x_VP_FRMR_0_CTRL_0 + (port_offset * frmr_sel), &value);
    value  &= 0xF0FF00FE;
    value  |= 0x1;
    value  |= (vpe_sel << 8);
    value  |= (frmr_schm << 14);
    value  |= (frmr_caps << 12);
    value  |= (frmr_que_len << 24);
    result &= writeDword(p_comm, CX2684x_VP_FRMR_0_CTRL_0 + (port_offset * frmr_sel), value);

    value   = (frmr_slot_len & 0xffff);
    value  |= ((frmr_slot_thr & 0xFFFF) << 16);
    result &= writeDword(p_comm, CX2684x_VP_FRMR_0_CTRL_1 + (port_offset * frmr_sel), value);

    result &= readDword(p_comm,  CX2684x_VP_FRMR_0_CTRL_3 + (port_offset * frmr_sel), &value);
    value  &= 0x0FFFFFFF;
    value  |= ((frmr_slot_len & 0x30000) << 12);           /* frmr_slot_len_hi bit[29:28] */
    value  |= ((frmr_slot_thr & 0x30000) << 14);           /* frmr_slot_thrshld_hi bit[31:30] */
    result &= writeDword(p_comm, CX2684x_VP_FRMR_0_CTRL_3 + (port_offset * frmr_sel), value);

    result &= writeDword(p_comm, CX2684x_VP_FRMR_0_CTRL_2 + (port_offset * frmr_sel), frmr_slot_que);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_setupLowFrameRate
 * Setup Cx2684x as low frame rate
 *
 *******************************************************************************************************/
int
CX2684x_setupLowFrameRate(const CX_COMMUNICATION *p_comm,
                       const unsigned long standard,
                       const unsigned char dvp_port,             /* port A, B, C, D */
                       unsigned long *src_sel,                   /* Video source/s */
                       const unsigned char strip_invalid_pixels)
{
    int i, result = TRUE;
    unsigned long value;

    unsigned long video_extend_mode = VIDEO_EXTEND_D1;

    unsigned long vpe_sel = 0x0;                       /* VPE_1 */
    /*unsigned long src_sel = 0;*/

    unsigned long output_mode = OMUXMODE_1_STREAM_REMUX;

    unsigned long frmr_sel  = VIDEO_FRAMER_0;
    unsigned long frmr_mode = VIDEO_D1_PAL_NORMAL_FRAMER_II;

    unsigned long frmr_caps     = 0x3;                 /* 4way */
    unsigned long frmr_schm     = 0x0;                 /* Fixed order round robin rotating */
    unsigned long frmr_que_len  = 0x0;
    unsigned long frmr_slot_que = 0x40404040;
    unsigned long frmr_slot_len = 0x13880;             /* PAL */
    unsigned long frmr_slot_thr = frmr_slot_len / 2;

    CX2684x_setVideoPackExtend(p_comm, vpe_sel, video_extend_mode);

    CX2684x_setVideoPackEn(p_comm, vpe_sel, src_sel[0], 0x0, 0x3, strip_invalid_pixels);

    CX2684x_setVideoPackFormat(p_comm, vpe_sel, standard);

    CX2684x_setVideoOMUX(p_comm, dvp_port, output_mode);

    CX2684x_setVideoFrmrFormat(p_comm, frmr_sel, frmr_mode);

    CX2684x_setVideoFrmrFormatTFVB(p_comm, frmr_sel, frmr_mode);

    for (i = 0; i < 4; i++)
    {
        CX2684x_setVideoFrmrMode(p_comm, frmr_sel, i, video_extend_mode, i * 2, i);
    }

    /* enable framer */
    CX2684x_enableVideoFramer(p_comm, frmr_sel, vpe_sel, frmr_caps, frmr_schm, frmr_que_len, frmr_slot_que, frmr_slot_len, frmr_slot_thr);

    /* soft reset for VP */
    result &= readDword(p_comm, CX2684x_CHIP_RST, &value );
    result &= writeDword(p_comm, CX2684x_CHIP_RST, value | 0x4 );

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_initializeMotionDetect
 * Setup Cx2684x motion detect block
 *
 *******************************************************************************************************/
int
CX2684x_initializeMotionDetect(const CX_COMMUNICATION *p_comm,
                            const PMOT_DET_INFO p_mot_det_info,
                            const unsigned long standard,
                            const int ch)
{
    int result = TRUE;

    unsigned long mot_msk = 0x0;
    int blk_num = 0;
    int blk_idx = 0;
    int i;

    unsigned long value = ( standard == NTSC_VIDEO_STD || standard == NTSC_EFFIO_STD ) ? 0x14 : 0x18;
    unsigned long xcnt = (standard == NTSC_EFFIO_STD || standard == PAL_EFFIO_STD) ? 0x2F : 0x2C;

    /* soft reset to channel bit[0] */
    unsigned long tmp;
    result &= readDword( p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), &tmp );
    result &= writeDword(p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), tmp | 0x1 );

    if (p_mot_det_info->mot_det_enable )
    {
        /* Set motion sensitivity threshold bit[7:0] to 5                       */
        /* set motion number of blocks detected threshold bit[15:8] to 4 blocks */
        result &= readDword( p_comm, CX2684x_MDET_0_MOTION_THRESH + (0x100 * ch), &tmp );
        tmp &= 0xFFFF0000;
        result &= writeDword(p_comm, CX2684x_MDET_0_MOTION_THRESH + (0x100 * ch), tmp | (p_mot_det_info->mot_det_block_thresh << 8) | p_mot_det_info->mot_det_sens_thresh );
    }

    if (p_mot_det_info->no_mot_det_enable )
    {
        /* Set still frame sensitivity threshold bit[7:0]                          */
        /* Set number of blocks required for still frame[15:8]                     */
        /* Set number of consecutive still frames required before interrupt[23:16] */
        result &= readDword( p_comm, CX2684x_MDET_0_STILL_THRESH + (0x100 * ch), &tmp );
        tmp &= 0xFF000000;
        tmp |= (p_mot_det_info->no_mot_det_sens_thresh & 0xFF);
        tmp |= ((p_mot_det_info->no_mot_det_block_thresh & 0xFF) << 8);
        tmp |= ((p_mot_det_info->no_mot_det_still_thresh & 0xFF) << 16);
        result &= writeDword(p_comm, CX2684x_MDET_0_STILL_THRESH + (0x100 * ch), tmp);

        /* should we set the motion thresh values???? just in case it wasn't set... */
        result &= readDword( p_comm, CX2684x_MDET_0_MOTION_THRESH + (0x100 * ch), &tmp );
        tmp &= 0xFFFF0000;
        result &= writeDword(p_comm, CX2684x_MDET_0_MOTION_THRESH + (0x100 * ch), tmp | (p_mot_det_info->mot_det_block_thresh << 8) | p_mot_det_info->mot_det_sens_thresh );

    }

    /* Set threshold for black pixel detection[7:0]                                          */
    /* Set black frame detected if number of "black" pixels is greater than this value[31:8] */
    if (p_mot_det_info->black_det_enable )
    {
        tmp = 0x00000000;
        tmp |= (p_mot_det_info->black_pixel_det_thresh & 0xFF);
        tmp |= ((p_mot_det_info->black_pixel_count_thresh & 0xFFFFFF) << 8);
        result &= writeDword(p_comm, CX2684x_MDET_0_BLACK_THRESH + (0x100 * ch), tmp);
    }

    /* Set threshold for white pixel detection[7:0]                                          */
    /* Set white frame detected if number of "white" pixels is greater than this value[31:8] */
    if (p_mot_det_info->white_det_enable )
    {
        tmp = 0x00000000;
        tmp |= (p_mot_det_info->white_pixel_det_thresh & 0xFF);
        tmp |= ((p_mot_det_info->white_pixel_count_thresh & 0xFFFFFF) << 8);
        result &= writeDword(p_comm, CX2684x_MDET_0_WHITE_THRESH + (0x100 * ch), tmp);
    }

    /* Set the skip frame count */
    result &= readDword( p_comm, CX2684x_MDET_0_SKIP + (0x100 * ch), &tmp );
    tmp &= 0xFFFFFF00;
    tmp |= (p_mot_det_info->mot_det_skip_n_frames & 0xFF);
    result &= writeDword(p_comm, CX2684x_MDET_0_SKIP + (0x100 * ch), tmp);

    for (i = 0; i < 6; i++)
    {
        /* disable motion for all blocks */
        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK0 + (0x100 * ch) + (0x4 * i), 0x0 );
    }

    /* disable motion for all blocks */
    for (i = 0; i < 2; i++)
    {
        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK6 + (0x100 * ch) + (0x4 * i), 0x0 );
    }

    /* enable motion for selected blocks since both Motion mask for D1 and Effio start from MOT_MASK7 */
    for (i = 1; i >= 0; i--)
    {
        result &= readDword( p_comm, CX2684x_MDET_0_MOT_MASK6 + (0x100 * ch) + (0x4 * i), &mot_msk );

        /* check the last 16 blocks [17 ... 32] */
        if( i == 0 )
        {
            for (blk_idx = 31; blk_idx >= 16; blk_idx--)
            {
                if ( p_mot_det_info->mot_det_blk_sel[blk_num] )
                {
                    mot_msk = setBitAtPos(mot_msk, (unsigned char)blk_idx);
                }

                blk_num++;
            }
        }


        /* check the first 16 blocks [1 ... 16] */
        for (blk_idx = 15; blk_idx >= 0; blk_idx--)
        {
            if ( p_mot_det_info->mot_det_blk_sel[blk_num] )
            {
                mot_msk = setBitAtPos(mot_msk, (unsigned char)blk_idx);
            }

            blk_num++;
        }

        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK6 + (0x100 * ch) + (0x4 * i), mot_msk );
    }

    /* enable motion for selected blocks */
    for (i = 5; i >= 0; i--)
    {
        result &= readDword( p_comm, CX2684x_MDET_0_MOT_MASK0 + (0x100 * ch) + (0x4 * i), &mot_msk );

        /* check the last 16 blocks [17 ... 32] */
        for (blk_idx = 31; blk_idx >= 16; blk_idx--)
        {
            if ( p_mot_det_info->mot_det_blk_sel[blk_num] )
            {
                mot_msk = setBitAtPos(mot_msk, (unsigned char)blk_idx);
            }

            blk_num++;
        }


        /* check the first 16 blocks [1 ... 16] */
        for (blk_idx = 15; blk_idx >= 0; blk_idx--)
        {
            if ( p_mot_det_info->mot_det_blk_sel[blk_num] )
            {
                mot_msk = setBitAtPos(mot_msk, (unsigned char)blk_idx);
            }

            blk_num++;
        }

        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK0 + (0x100 * ch) + (0x4 * i), mot_msk );
    }

    result &= readDword( p_comm, CX2684x_MDET_0_GRID_XCNT + (0x100 * ch), &tmp );
    tmp &= 0xFFFFFF00;
    result &= writeDword(p_comm, CX2684x_MDET_0_GRID_XCNT + (0x100 * ch), tmp | xcnt );

    result &= readDword( p_comm, CX2684x_MDET_0_GRID_YCNT + (0x100 * ch), &tmp );
    tmp &= 0xFFFFFF00;
    result &= writeDword(p_comm, CX2684x_MDET_0_GRID_YCNT + (0x100 * ch), tmp | value );

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_startMotionDetect
 * Start Cx2684x motion detect block
 *
 *******************************************************************************************************/
int
CX2684x_startMotionDetect(const CX_COMMUNICATION *p_comm,
                       const PMOT_DET_INFO p_mot_det_info,
                       const int ch,
                       unsigned long *mot_det_intr_msk)
{
    int result = TRUE;
    unsigned long tmp;
    unsigned long int_msk_value;
    unsigned long int_msk_value1;

    unsigned long int_stat1 = 0;
    result &= readDword( p_comm, CX2684x_INT_STAT1, &int_stat1 );
    result &= writeDword(p_comm, CX2684x_INT_STAT1, int_stat1);  //Clear INT_STAT1

    // enable motion detected interrupt bit[1]
    result &= readDword( p_comm, CX2684x_MDET_0_INTR_MSK + (0x100 * ch), &tmp );

    if (p_mot_det_info->mot_det_enable )      // enable motion detection
       tmp &= 0xFFFFFFFD;

    if (p_mot_det_info->no_mot_det_enable )   // enable no motion detection
       tmp &= 0xFFFFFFFB;

    if (p_mot_det_info->black_det_enable )    // enable black detection
       tmp &= 0xFFFFFFF7;

    if (p_mot_det_info->white_det_enable )    // enable white detection
       tmp &= 0xFFFFFFEF;

    result &= writeDword(p_comm, CX2684x_MDET_0_INTR_MSK + (0x100 * ch), tmp);
    *mot_det_intr_msk = tmp;

    // enable this channel bit[2]
    result &= readDword( p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), &tmp );
    result &= writeDword(p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), tmp | 0x4 );

    // mask all interrupts
    result &= readDword( p_comm, CX2684x_INT_MSK0, &tmp);
    int_msk_value = tmp;
    result &= writeDword(p_comm, CX2684x_INT_MSK0, 0xffffffff);

    result &= readDword( p_comm, CX2684x_INT_MSK1, &tmp);
    int_msk_value1 = tmp;
    result &= writeDword(p_comm, CX2684x_INT_MSK1, 0xffffffff);

    // set irq polarity active low
    result &= readDword(p_comm, CX2684x_IRQ_CTRL, &tmp);
    result &= writeDword(p_comm, CX2684x_IRQ_CTRL, tmp | 0x02);

    // enable irq as output
    result &= readDword( p_comm, CX2684x_CLK_CTRL1, &tmp);
    result &= writeDword(p_comm, CX2684x_CLK_CTRL1, tmp | 0x80000000);

    // return all int msk
    tmp = int_msk_value & (~(1 << (ch + 8)));
    result &= writeDword(p_comm, CX2684x_INT_MSK0, tmp);
    result &= writeDword(p_comm, CX2684x_INT_MSK1, int_msk_value1);

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_stopMotionDetect
 * Stop Cx2684x motion detect block
 *
 *******************************************************************************************************/
int
CX2684x_stopMotionDetect(const CX_COMMUNICATION *p_comm,
                      const int ch)
{
    int result = TRUE;
    int i;
    unsigned long tmp;

    /* disable this channel bit[2] */
    result &= readDword( p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), &tmp );
    result &= writeDword(p_comm, CX2684x_MDET_0_CTRL + (0x100 * ch), tmp & 0xB );

    // disable all motion detected interrupt    bit[1]
    result &= readDword( p_comm, CX2684x_MDET_0_INTR_MSK + (0x100 * ch), &tmp );
    result &= writeDword(p_comm, CX2684x_MDET_0_INTR_MSK + (0x100 * ch), tmp | 0x1E );

    for (i = 0; i < 6; i++)
    {
        // disable motion for all blocks
        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK0 + (0x100 * ch) + (0x4 * i), 0x0 );
    }

    for (i = 0; i < 2; i++)
    {
        // disable motion for all blocks
        result &= writeDword( p_comm, CX2684x_MDET_0_MOT_MASK6 + (0x100 * ch) + (0x4 * i), 0x0 );
    }

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_getMotionStatus
 * Get Cx2684x motion status
 *
 * Block  1                                 16
 *       |------------------------------------|
 *       |31|30|                     |18|17|16|     MDET_0_MOT_STAT5 [31:0]
 *       |------------------------------------|
 *       |15|14|                     | 2| 1| 0|
 *       |------------------------------------|
 *       |                                    |
 *       |                                    |
 *       |                                    |
 *       |------------------------------------|
 *       |31|30|                     |18|17|16|     MDET_0_MOT_STAT0 [31:0]
 *       |------------------------------------|
 *       |15|14|                     | 2| 1| 0|
 *       |------------------------------------|
 * Block  177                              192
 *******************************************************************************************************/
static int
CX2684x_getMotionStatus(const CX_COMMUNICATION *p_comm,
                     const PMOT_DET_INFO p_mot_det_info,
                     const unsigned long standard,
                     const int ch)
{
    int result = TRUE;
    int got_motion = 0;

    if ( standard == NTSC_EFFIO_STD || standard == PAL_EFFIO_STD )
    {
        result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT7 + (0x100 * ch), &p_mot_det_info->mot_stat7 );
        p_mot_det_info->mot_stat7 &= 0xFFFF; //only blocks [1 ... 16]

        result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT6 + (0x100 * ch), &p_mot_det_info->mot_stat6 );
    }

    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT5 + (0x100 * ch), &p_mot_det_info->mot_stat5 );
    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT4 + (0x100 * ch), &p_mot_det_info->mot_stat4 );
    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT3 + (0x100 * ch), &p_mot_det_info->mot_stat3 );
    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT2 + (0x100 * ch), &p_mot_det_info->mot_stat2 );
    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT1 + (0x100 * ch), &p_mot_det_info->mot_stat1 );
    result &= readDword( p_comm, CX2684x_MDET_0_MOT_STAT0 + (0x100 * ch), &p_mot_det_info->mot_stat0 );

    got_motion = (p_mot_det_info->mot_stat7 | p_mot_det_info->mot_stat6 | p_mot_det_info->mot_stat5 | p_mot_det_info->mot_stat4 |
                  p_mot_det_info->mot_stat3 | p_mot_det_info->mot_stat2 | p_mot_det_info->mot_stat1 | p_mot_det_info->mot_stat0 ) ? 1 : 0;
    p_mot_det_info->mot_num_blks_detected = got_motion;

    return result;
}

/*******************************************************************************************************
 *
 * CX2684x_processMotionDetect
 * Process Cx2684x motion detection
 *
 *******************************************************************************************************/
int
CX2684x_processMotionDetect(const CX_COMMUNICATION *p_comm,
                         const PMOT_DET_INFO p_mot_det_info,
                         const unsigned long standard,
                         const int ch)
{
    const unsigned long FLD_MOT_DET_INT    = (1 << 1);
    const unsigned long FLD_NO_MOT_DET_INT = (1 << 2);
    const unsigned long FLD_BLACK_DET_INT  = (1 << 3);
    const unsigned long FLD_WHITE_DEC_INT  = (1 << 4);

    int result = TRUE;
    unsigned long intr_msk;
    unsigned long intr_stat;

    result &= readDword( p_comm, CX2684x_MDET_0_INTR_MSK  + (0x100 * ch), &intr_msk);
    result &= readDword( p_comm, CX2684x_MDET_0_INTR_STAT + (0x100 * ch), &intr_stat );

    /* Read and write back the interrupt status register to clear it */
    result &= writeDword( p_comm, CX2684x_MDET_0_INTR_STAT + (0x100 * ch), intr_stat);

    if ((~intr_msk & intr_stat & FLD_MOT_DET_INT ) != 0)
    {
        CX2684x_getMotionStatus( p_comm, p_mot_det_info, standard, ch );
    }
    if ((~intr_msk & intr_stat & FLD_NO_MOT_DET_INT) != 0)
    {
        p_mot_det_info->no_mot_detected = 1;
    }
    if ((~intr_msk & intr_stat & FLD_BLACK_DET_INT) != 0)
    {
        p_mot_det_info->black_detected = 1;
    }
    if ((~intr_msk & intr_stat & FLD_WHITE_DEC_INT) != 0)
    {
        p_mot_det_info->white_detected = 1;
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_setDRE()
 *
 ******************************************************************************/
int
CX2684x_setDRE(const CX_COMMUNICATION *p_comm,
            unsigned long *coe_curve,
            const unsigned char decoder)
{
    int result = TRUE;
    int i = 0;
    unsigned long en_state;
    unsigned long addr_offset;
    unsigned long addr_start;
    unsigned long wr_data;

    /* This bitmap indicates which channel will go through DRE module. */
    unsigned long channel_bitmap[8] =
    {
        0x0,   /* Bit 0:  Video Decoder A */
        0x2,   /* Bit 2:  Video Decoder B */
        0x4,   /* Bit 4:  Video Decoder C */
        0x6,   /* Bit 6:  Video Decoder D */
        0x8,   /* Bit 8:  Video Decoder E */
        0xA,   /* Bit 10: Video Decoder F */
        0xC,   /* Bit 12: Video Decoder G */
        0xE    /* Bit 14: Video Decoder H */
    };

    /* disable DRE */
    result &= readDword( p_comm, CX2684x_VP_DRE_0, &en_state );
    result &= writeDword(p_comm, CX2684x_VP_DRE_0, 0 );

    addr_offset = decoder << 8;
    addr_start  = addr_offset;
    wr_data     = 0;

    result &= writeDword(p_comm, CX2684x_VP_DRE_1, ( (0x3 << 28) + (addr_start << 16) + 0x0)  );

    for (i = 1; i < 257; i++)
    {
        wr_data = wr_data + (coe_curve[i-1] << ( ( (i-1) % 4 ) * 8 ) );

        if ( (i % 4) == 0 )
        {
            result &= writeDword(p_comm, CX2684x_VP_DRE_2, wr_data );
            wr_data = 0;
        }
    }

    /* enable DRE */
    result &= writeDword(p_comm, CX2684x_VP_DRE_0, (0x3 << channel_bitmap[decoder] ) | en_state );

    return result;
}

/******************************************************************************
 *
 * CX2684x_disableDRE()
 *
 ******************************************************************************/
int
CX2684x_disableDRE(const CX_COMMUNICATION *p_comm,
                const unsigned char decoder)
{
    int result = TRUE;
    unsigned long value;

    /* This bitmap indicates which channel will go through DRE module. */
    unsigned long channel_bitmap[8] =
    {
        0x0,   /* Bit 0:  Video Decoder A */
        0x2,   /* Bit 2:  Video Decoder B */
        0x4,   /* Bit 4:  Video Decoder C */
        0x6,   /* Bit 6:  Video Decoder D */
        0x8,   /* Bit 8:  Video Decoder E */
        0xA,   /* Bit 10: Video Decoder F */
        0xC,   /* Bit 12: Video Decoder G */
        0xE    /* Bit 14: Video Decoder H */
    };

    /* disable DRE */
    result &= readDword( p_comm, CX2684x_VP_DRE_0, &value );
    value  &= ~(0x1 << channel_bitmap[decoder]);
    result &= writeDword(p_comm, CX2684x_VP_DRE_0, value );

    return result;
}

/******************************************************************************
 *
 * CX2684x_setVideoQuality()
 *
 ******************************************************************************/
int
CX2684x_setVideoQuality(const CX_COMMUNICATION *p_comm,
                     const unsigned long standard,
                     const unsigned long video_quality,
                     const unsigned char decoder)
{
    int result = TRUE;

    /* COMB FILTER SETTING Below */
    if (standard == PAL_VIDEO_STD || standard == PAL_EFFIO_STD)
    {
        if ( video_quality == BETTER_DOT ) //BETTER_DOT
        {
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x00018336);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10A00C90);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x30101030);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E090000);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);
        }
        else if ( video_quality == BETTER_ZONEPLATE ) //BETTER_ZONE_PLATE
        {
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x0001D326);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10A00C90);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x3010100A);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E190000);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);
        }
        else if ( video_quality == LESSER_DOT ) //LESSER_DOT
        {
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x0001D326);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10040C90);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x30161014);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E290000);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);
        }
        else if ( video_quality == BEST_DOT ) //BEST_DOT
        {
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x00015326);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10060C90);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x18661090);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E190000);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);
        }
        else //Default
        {
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL1 +(0x200*decoder), 0x0001D326);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL2 +(0x200*decoder), 0x10900C90);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL3 +(0x200*decoder), 0x30061014);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_FILT_CTRL4 +(0x200*decoder), 0x1E190000);
            result &= writeDword(p_comm, CX2684x_VDECA_COMB_BLEND_CTRL +(0x200*decoder), 0x00202060);
        }
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_writeRegister()
 *
 ******************************************************************************/
int
CX2684x_writeRegister(const CX_COMMUNICATION *p_comm,
                const unsigned long registerAddress,
                const unsigned long value)
{
    int result = TRUE;

    result &= writeDword(p_comm, registerAddress, value );

    return result;
}

/******************************************************************************
 *
 * CX2684x_readRegister()
 *
 ******************************************************************************/
int
CX2684x_readRegister(const CX_COMMUNICATION *p_comm,
               const unsigned long registerAddress,
               unsigned long *p_value)
{
    int result = TRUE;

    result &= readDword( p_comm, registerAddress, p_value );

    return result;
}

/******************************************************************************
 *
 * CX2684x_setLowPassFilter()
 *
 ******************************************************************************/
int
CX2684x_setLowPassFilter(const CX_COMMUNICATION *p_comm,
                     const unsigned long lpf_sel,
                     const unsigned char decoder)
{
    int result = TRUE;
    unsigned long value;

    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF0+(0x200*decoder), LPF_Coeff[lpf_sel][0] );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF1+(0x200*decoder), LPF_Coeff[lpf_sel][1] );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF2+(0x200*decoder), LPF_Coeff[lpf_sel][2] );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF3+(0x200*decoder), LPF_Coeff[lpf_sel][3] );

    /* enable equalizer */
    result &= readDword( p_comm, CX2684x_VDECA_SRC_CFG + (0x200 * decoder), &value );
    value |= 0x00010000;
    result &= writeDword(p_comm, CX2684x_VDECA_SRC_CFG + (0x200 * decoder), value );

    return result;
}

/******************************************************************************
 *
 * CX2684x_disableLowPassFilter()
 *
 ******************************************************************************/
int
CX2684x_disableLowPassFilter(const CX_COMMUNICATION *p_comm,
                         const unsigned char decoder)
{
    int result = TRUE;
    unsigned long value;

    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF0 + (0x200 * decoder), 0x0 );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF1 + (0x200 * decoder), 0x0 );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF2 + (0x200 * decoder), 0x0 );
    result &= writeDword(p_comm, CX2684x_VDECA_EQ_FIR_COEFF3 + (0x200 * decoder), 0x0 );

    /* disable equalizer */
    result &= readDword( p_comm, CX2684x_VDECA_SRC_CFG + (0x200 * decoder), &value );
    value &= 0xfffeffff;
    result &= writeDword(p_comm, CX2684x_VDECA_SRC_CFG + (0x200 * decoder), value );

    return result;
}

/******************************************************************************
 *
 * CX2684x_setBigEndianDVP()
 *
 ******************************************************************************/
int
CX2684x_setBigEndianDVP(const CX_COMMUNICATION *p_comm,
                     const unsigned char big_endian_en)   /* 0 = little-endian, 1 = big-endian */
{
    int i = 0;
    int result = TRUE;
    unsigned long value;

    for (i = 0; i < 4; i++)
    {
        result &= readDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * i), &value );
        value  &= 0xfffffff7;
        value  |= (big_endian_en << 3);
        result &= writeDword(p_comm, CX2684x_VP_OMUX_A + (0x04 * i), value );
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_initializePtz()
 *
 ******************************************************************************/
int
CX2684x_initializePTZ(const CX_COMMUNICATION *p_comm,
                      const unsigned char decoder,
                      int is_effio, int chip_num)
{
    int result = TRUE;
    unsigned long value = 0x0;
    unsigned long ptz_offset         = 0x0;
    unsigned long ptz_sub_bit_width3 = 0x0;
    unsigned long ptz_sub_bit_width2 = 0x0;
    unsigned long ptz_sub_bit_width1 = 0x0;
    unsigned long ptz_ln1 = 0x13; //default = 0x13 (2nd line of 32bit PTZ)
    unsigned long ptz_ln0 = 0x0F; //NTSC D1 works between 0xC to 0x10 (line number)
    unsigned long ptz_fld = 0x2;  //Field selection of PTZ default = 0x2 for both fields
    unsigned long ptz_type = 0x0; //15bit PTZ protocol


    if (is_effio == 1)
    {
        ptz_offset          = 0x600;    //default (1/144MHz * 1536 cycles) = 10.667 us
        ptz_sub_bit_width3  = 0x90;     //(1/144Mhz * 144 cycles) ~= 1us This field defines ptz sub bit width, generally 1us
        ptz_sub_bit_width2  = 0x90;
        ptz_sub_bit_width1  = 0x90;
    }
    else
    {
        ptz_offset          = 0x480;    // (1/108MHz * 1152 cycles) = 10.667 us
        ptz_sub_bit_width3  = 0x6C;     //(1/108Mhz * 108 cycles) ~= 1us This field defines ptz sub bit width, generally 1us
        ptz_sub_bit_width2  = 0x6C;
        ptz_sub_bit_width1  = 0x6C;
    }

    //Decoder field: defines which video source is being controlled 0~3: internal decoder A-D, 4~15: external decoders
    result &= readDword(p_comm, CX2684x_PTZ_CTRL1, &value );
    value &= 0xe00e0000;    //clear bits 0-16, bits 20-28
    result &= writeDword(p_comm, CX2684x_PTZ_CTRL1, value | (ptz_ln1 << 20) | (ptz_ln0 << 8) | (ptz_fld << 6) | (ptz_type << 4) | decoder);

    result &= readDword(p_comm, CX2684x_PTZ_CTRL2, &value );
    value &= 0x0000FFFF;    //clear bits 16-31 //This field defines the offset cycles between hsync and ptz_data (108M/144M depends on decoder working mode)
    result &= writeDword(p_comm, CX2684x_PTZ_CTRL2, value | (ptz_offset << 16) );

    result &= readDword(p_comm, CX2684x_PTZ_CTRL3, &value );
    value &= 0xFF000000;    //clear bits 0-23 (sub bit width)
    result &= writeDword(p_comm, CX2684x_PTZ_CTRL3, value | (ptz_sub_bit_width3 << 16) | (ptz_sub_bit_width2 << 8) | ptz_sub_bit_width1 );

    result &= readDword(p_comm, CX2684x_PTZ_CTRL2, &value );

	if (chip_num == 0)
	{
    	result &= writeDword(p_comm, CX2684x_PTZ_CTRL2, value | 0x1 | (0x1 << 1)); //enable ptz and cascade.
    }
	else
	{
		result &= writeDword(p_comm, CX2684x_PTZ_CTRL2, value | 0x1 ); //enable ptz
	}

	result &= readDword(p_comm, CX2684x_AFE_DIAG_CTRL, &value );

	if (chip_num == 0)
	{
		result &= writeDword(p_comm, CX2684x_AFE_DIAG_CTRL, value | (0x1 << 12)); //enable pin remap
	}
	else
	{
		result &= writeDword(p_comm, CX2684x_AFE_DIAG_CTRL, value | (0x1 << 10)); //invert the PTZ output.
	}



    return result;
}

/******************************************************************************
 *
 * CX2684x_sendPtzCommand()
 *
 ******************************************************************************/
int
CX2684x_sendPtzCommand(const CX_COMMUNICATION *p_comm,
                       const unsigned long coaxitron_cmd)
{
    int i = 0;
    int result = TRUE;
    unsigned long value = 0x0;

    result &= readDword(p_comm, CX2684x_PTZ_DAT0, &value );
    value &= 0x00010001;    //clear bits 1-15, bits 17-31
    result &= writeDword(p_comm, CX2684x_PTZ_DAT0, value | (coaxitron_cmd << 1) | (coaxitron_cmd << 17) );

    result &= readDword(p_comm, CX2684x_PTZ_DAT1, &value );
    value &= 0x00010001;    //clear bits 1-15, bits 17-31
    result &= writeDword(p_comm, CX2684x_PTZ_DAT1, value | (coaxitron_cmd << 1) | (coaxitron_cmd << 17) );

    result &= readDword(p_comm, CX2684x_PTZ_DAT2, &value );
    value &= 0x00010001;    //clear bits 1-15, bits 17-31
    result &= writeDword(p_comm, CX2684x_PTZ_DAT2, value | (coaxitron_cmd << 1) | (coaxitron_cmd << 17) );

    result &= readDword(p_comm, CX2684x_PTZ_CTRL0, &value );

    for (i = 0; i < 500; i++ )
    {
        result &= writeDword(p_comm, CX2684x_PTZ_CTRL0, value | 0x1 );  //start PTZ
    }

    return result;
}

/******************************************************************************
 *
 * CX2684x_getVideoLostStatus()
 *
 ******************************************************************************/
int CX2684x_getVideoLostStatus(const CX_COMMUNICATION *p_comm,
                               const unsigned char decoder, int *lostStatus)
{
    int result = TRUE;
    unsigned long value;
    unsigned long src_vpres = 0;
    unsigned long src_lock = 0;

    result &= readDword( p_comm, CX2684x_VDECA_GEN_STAT + (0x200 * decoder), &value);
    src_vpres = (value & 0x00004000) >> 14;
    src_lock  = (value & 0x00000020) >> 5;
    *lostStatus = ((src_vpres & src_lock) == 1) ? 0 : 1;

    return result;
}

#if 0
/******************************************************************************
 *
 * CX2684x_getDeviceId()
 *
 ******************************************************************************/
static int
CX2684x_getDeviceId(const CX_COMMUNICATION *p_comm)
{
    unsigned long dev_id = 0;
    readDword(p_comm, (unsigned long) CX2684x_DEV_REV_ID, &dev_id);
    dev_id = (dev_id & 0x0000FFFF);

    return dev_id;
}
#endif
