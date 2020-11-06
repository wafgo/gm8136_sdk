#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ioctl_isp320.h"

static int isp_fd;

static int get_choose_int(void)
{
    int val;
    int error;
    char buf[256];

    error = scanf("%d", &val);
    if (error != 1) {
        printf("Invalid option. Try again.\n");
        clearerr(stdin);
        fgets(buf, sizeof(buf), stdin);
        val = -1;
    }

    return val;
}

static unsigned int get_choose_hex(void)
{
    unsigned int val;
    int error;
    char buf[256];

    error = scanf("%x", &val);
    if (error != 1) {
        printf("Invalid input. Try again.\n");
        clearerr(stdin);
        fgets(buf, sizeof(buf), stdin);
        val = -1;
    }

    return val;
}

static int get_func0_sts(void)
{
    u32 func0_sts;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_FUNC0_STS, &func0_sts);
    if (ret < 0)
        return ret;
    printf("FUNC0 status = 0x%08x\n", func0_sts);
    return ret;
}

static int enable_func0(void)
{
    u32 func0_en;

    do {
        printf("enable FUNC0 >> 0x");
        func0_en = get_choose_hex();
    } while (func0_en < 0);

    func0_en &= FUNC0_ALL;
    return ioctl(isp_fd, ISP_IOC_ENABLE_FUNC0, &func0_en);
}

static int disable_func0(void)
{
    u32 func0_en;

    do {
        printf("disable FUNC0 >> 0x");
        func0_en = get_choose_hex();
    } while (func0_en < 0);

    func0_en &= FUNC0_ALL;
    return ioctl(isp_fd, ISP_IOC_DISABLE_FUNC0, &func0_en);
}

static int get_func1_sts(void)
{
    u32 func1_sts;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_FUNC1_STS, &func1_sts);
    if (ret < 0)
        return ret;
    printf("FUNC1 status = 0x%08x\n", func1_sts);
    return ret;
}

static int enable_func1(void)
{
    u32 func1_en;

    do {
        printf("enable FUNC1 >> 0x");
        func1_en = get_choose_hex();
    } while (func1_en < 0);

    func1_en &= FUNC1_ALL;
    return ioctl(isp_fd, ISP_IOC_ENABLE_FUNC1, &func1_en);
}

static int disable_func1(void)
{
    u32 func1_en;

    do {
        printf("disable FUNC0 >> 0x");
        func1_en = get_choose_hex();
    } while (func1_en < 0);

    func1_en &= FUNC1_ALL;
    return ioctl(isp_fd, ISP_IOC_DISABLE_FUNC1, &func1_en);
}

static int test_func_en(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Get FUNC0 status\n");
        printf(" 2. Enable FUNC0\n");
        printf(" 3. Disable FUNC0\n");
        printf(" 4. Get FUNC1 status\n");
        printf(" 5. Enable FUNC1\n");
        printf(" 6. Disable FUNC1\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            get_func0_sts();
            break;
        case 2:
            enable_func0();
            break;
        case 3:
            disable_func0();
            break;
        case 4:
            get_func1_sts();
            break;
        case 5:
            enable_func1();
            break;
        case 6:
            disable_func1();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int test_blc(void)
{
    const char *c_name[] = {"R", "Gr", "Gb", "B"};
    blc_reg_t blc;
    int adjust_blc = 0;
    int ret, i;

    // disable auto BLC adjustment
    ret = ioctl(isp_fd, ISP_IOC_SET_ADJUST_BLC_EN, &adjust_blc);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_BLC, &blc);
    if (ret < 0)
        return ret;
    printf("BLC: %d, %d, %d, %d\n", blc.offset[0], blc.offset[1],
                                    blc.offset[2], blc.offset[3]);
    for (i=0; i<4; i++) {
        printf("BLC %s offset >> ", c_name[i]);
        blc.offset[i] = get_choose_int();
    }

    return ioctl(isp_fd, ISP_IOC_SET_BLC, &blc);
}

static int test_li(void)
{
    li_lut_t li_lut;
    li_lut_t li_lut0 = {
        {16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,255},
        {16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,255},
        {16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,255},
    };

    li_lut_t li_lut1 = {
        {4,11,20,32,44,58,74,90,108,126,146,166,187,209,232,255},
        {4,11,20,32,44,58,74,90,108,126,146,166,187,209,232,255},
        {4,11,20,32,44,58,74,90,108,126,146,166,187,209,232,255},
    };

    int i, opt, ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_LI, &li_lut);
    if (ret < 0)
        return ret;
    printf("rlut:");
    for (i=0; i<16; i++)
        printf("0%d, ", li_lut.rlut[i]);
    printf("\n");

    printf("glut:");
    for (i=0; i<16; i++)
        printf("0%d, ", li_lut.glut[i]);
    printf("\n");

    printf("blut:");
    for (i=0; i<16; i++)
        printf("0%d, ", li_lut.blut[i]);
    printf("\n");


    do {
        printf("Gamma (0:LI_TAB0, 1:LI_TAB1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_LI, &li_lut0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_LI, &li_lut1);
    }

    return ret;
}

static int test_dpc(void)
{
    dpc_reg_t dpc_reg;
    int ret = 0, i;

    ret =ioctl(isp_fd, ISP_IOC_GET_DPC, &dpc_reg);
    if (ret < 0)
        return ret;
    printf("dpc_mode = %d\n", dpc_reg.dpc_mode);
    printf("rvc_mode = %d\n", dpc_reg.rvc_mode);
    for (i=0; i<3; i++)
        printf("th[%d] = %d\n", i, dpc_reg.th[i]);

    dpc_reg.dpc_mode = 0;
    dpc_reg.rvc_mode = 0;
    dpc_reg.dpcn  = 128;
    dpc_reg.th[0] = 1024;
    dpc_reg.th[1] = 96;
    dpc_reg.th[2] = 4;
    ret = ioctl(isp_fd, ISP_IOC_SET_DPC, &dpc_reg);

    return 0;
}

static int test_ctk(void)
{
    ctk_reg_t ctk_reg;
    int ret = 0, i;

    ret =ioctl(isp_fd, ISP_IOC_GET_RAW_CTK, &ctk_reg);
    if (ret < 0)
        return ret;
    for (i=0; i<4; i++)
        printf("th[%d] = %d\n", i, ctk_reg.th[i]);
    for (i=0; i<4; i++)
        printf("w[%d] = %d\n", i, ctk_reg.w[i]);
    printf("gbgr = %d\n", ctk_reg.gbgr);

    ctk_reg.th[0] = 23;
    ctk_reg.th[1] = 33;
    ctk_reg.th[2] = 40;
    ctk_reg.th[3] = 47;
    ctk_reg.w[0] = 1;
    ctk_reg.w[1] = 2;
    ctk_reg.w[2] = 3;
    ctk_reg.w[3] = 4;
    ctk_reg.gbgr = 3;

    ret = ioctl(isp_fd, ISP_IOC_SET_RAW_CTK, &ctk_reg);
    return 0;
}

static int test_lsc(void)
{
    const char *c_name[] = {"R", "Gr", "Gb", "B"};
    lsc_reg_t lsc;
    lsc_reg_t lsc_test = {
        15,
        { {542, 569}, {539, 573}, {540, 572}, {538, 568} },
        { {152,430,382,171,136,478,539,121},
          {154,424,382,171,134,486,539,121},
          {153,426,381,171,135,483,540,121},
          {153,427,386,169,135,482,533,122},
        },

        { {256,257,257,257,258,259,261,262,264,265,267,268,270,271,272,274,275,277,279,280,282,285,286,288,290,293,295,297,299,302,305,306,
           256,257,257,258,259,260,262,263,264,265,267,268,270,271,272,273,275,276,278,278,279,280,282,284,286,290,292,295,297,299,303,305,
           256,257,258,259,261,262,264,265,267,268,269,271,271,271,272,272,273,273,273,273,274,274,274,275,275,276,276,277,278,278,280,280,
           256,258,259,261,262,264,266,268,270,271,273,275,278,279,281,283,285,287,289,291,293,295,296,298,300,302,305,308,312,315,319,321,
           256,258,260,262,263,265,267,269,271,273,275,277,279,281,283,285,287,289,291,294,296,299,301,303,304,305,306,307,308,308,310,310,
           256,258,260,262,264,265,267,269,271,273,275,277,279,281,283,285,287,289,290,292,294,295,297,299,301,304,307,310,315,317,321,323,
           256,257,258,260,261,263,264,266,266,266,267,267,267,268,268,268,269,269,269,270,270,270,271,271,272,272,273,273,274,274,276,276,
           256,257,257,258,259,261,262,263,264,265,267,268,270,271,273,275,276,277,278,279,281,283,285,287,290,292,294,296,298,299,302,303
          },

          {256,257,257,258,258,259,260,261,262,263,264,266,267,269,270,271,272,273,274,276,277,279,280,282,283,285,286,288,290,291,294,294,
           256,257,257,258,258,259,260,261,262,263,264,266,266,267,268,269,270,271,273,273,274,275,276,278,279,281,283,285,287,288,291,292,
           256,257,257,258,259,260,261,262,263,264,266,266,266,266,267,267,267,267,267,268,268,268,268,269,269,269,270,270,271,271,272,273,
           256,257,257,258,259,261,262,263,265,266,268,269,270,272,273,274,275,276,277,278,279,280,281,282,284,285,287,289,292,294,298,298,
           256,257,257,259,260,261,263,264,266,267,269,270,272,273,274,275,276,277,279,280,282,283,285,287,289,290,290,291,292,292,294,294,
           256,257,257,259,260,261,263,265,266,268,269,270,271,272,273,275,276,277,278,279,280,281,282,283,285,288,290,292,295,297,301,302,
           256,257,257,258,259,260,262,262,262,262,263,263,263,263,264,264,264,264,264,265,265,265,265,266,266,266,267,267,268,268,270,270,
           256,257,257,258,258,259,261,261,262,263,264,266,267,269,270,271,272,273,273,274,275,277,279,280,282,283,284,286,288,290,292,293
          },

          {256,257,257,258,258,259,260,261,262,263,264,266,267,268,270,270,272,273,274,275,277,278,280,281,282,284,286,287,289,291,293,294,
           256,257,257,258,258,259,260,261,262,263,264,265,266,267,268,269,270,271,272,273,274,275,276,277,279,281,283,284,286,288,291,292,
           256,257,257,258,258,259,260,261,262,263,264,266,266,266,267,267,267,267,267,268,268,268,268,268,269,269,270,270,270,271,273,273,
           256,257,257,259,259,260,261,263,264,266,267,269,270,272,273,274,275,276,277,278,279,280,281,282,284,286,288,289,292,295,298,300,
           256,257,257,259,260,261,262,264,266,267,269,270,272,273,274,275,277,278,279,280,282,283,285,287,289,290,291,291,292,292,294,294,
           256,257,257,259,260,261,263,265,266,268,269,270,271,272,273,275,276,278,279,280,281,282,283,284,286,288,291,293,296,298,302,303,
           256,257,257,258,259,260,262,263,263,263,263,264,264,264,264,265,265,265,265,266,266,266,266,267,267,267,268,268,269,269,270,271,
           256,257,257,258,258,259,261,262,262,263,264,266,267,269,270,271,272,273,273,274,275,276,278,280,282,283,285,285,287,288,291,292
          },

          {256,258,259,260,261,262,263,264,266,267,269,271,273,274,276,278,280,282,283,285,287,288,290,291,293,295,297,299,301,303,306,306,
           256,258,258,260,260,261,262,263,265,267,268,270,271,273,275,277,278,280,280,281,282,283,284,286,288,290,292,295,297,298,301,303,
           256,258,258,259,259,260,262,263,265,267,268,270,270,270,271,271,271,272,272,272,272,272,273,273,273,274,274,275,275,276,278,278,
           256,258,258,259,260,261,262,264,266,268,270,271,273,275,277,278,280,281,282,283,284,285,286,287,289,290,293,295,298,302,307,308,
           256,258,258,259,261,262,263,265,267,269,271,272,275,277,278,279,281,282,284,285,287,289,291,293,294,296,297,297,298,299,300,301,
           256,258,259,260,262,263,264,266,268,269,271,273,275,277,278,279,281,282,283,284,286,287,288,289,291,293,297,300,303,306,310,312,
           256,258,259,260,262,263,264,265,266,266,267,267,267,268,268,268,269,269,269,269,270,270,270,271,271,271,272,272,273,273,275,275,
           256,258,259,260,261,262,263,264,266,268,270,272,274,275,277,279,280,281,283,284,286,287,289,291,293,295,296,297,299,301,305,306,
          },
        },
    };
    int ret = 0, i, j;

    ret = ioctl(isp_fd, ISP_IOC_GET_LSC, &lsc);
    if (ret < 0)
        return ret;

    printf("segrds = %d\n", lsc.segrds);
    for (i=0; i<4; i++)
        printf("%s center (%d, %d)\n", c_name[i], lsc.center[i].x, lsc.center[i].y);

    for (j=0; j<4; j++) {
        printf("lsc_mxtn_%s = ", c_name[j]);
        for (i=0; i<8; i++)
            printf("%d ", lsc.maxtan[j][i]);
        printf("\n");
    }

    for (j=0; j<4; j++) {
        printf("lsc_rdsparam_%s = ", c_name[j]);
        for (i=0; i<16; i++)
            printf("%d ", lsc.radparam[j][i]);
        printf("...\n");
    }

    return ioctl(isp_fd, ISP_IOC_SET_LSC, &lsc_test);
}

static int test_isp_gain(void)
{
    u32 isp_gain;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_ISP_GAIN, &isp_gain);
    if (ret < 0)
        return ret;
    printf("ISP Gain = %d\n", isp_gain);

    do {
        printf("ISP Gain (UFIX 9.7) >> ");
        isp_gain = get_choose_int();
    } while ((int)isp_gain < 0);

    ret = ioctl(isp_fd, ISP_IOC_SET_ISP_GAIN, &isp_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_ISP_GAIN fail\n");
        return ret;
    }

    return 0;
}

static int test_cg_format(void)
{
    int cg_format;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_CG_FORMAT, &cg_format);
    if (ret < 0) {
        printf("ISP_IOC_GET_CG_FORMAT fail\n");
        return ret;
    }

    printf("Color gain fomat is %s\n", cg_format ? "UFIX_3_6" : "UFIX_2_7");

    do {
        printf("Color gain fomat >> ");
        cg_format = get_choose_int();
    } while ((cg_format < 0) || (cg_format > 1));

    ret = ioctl(isp_fd, ISP_IOC_SET_CG_FORMAT, &cg_format);
    if (ret < 0) {
        printf("ISP_IOC_SET_CG_FORMAT fail\n");
        return ret;
    }

    return 0;
}

static int test_rgb_gain(void)
{
    u32 r_gain, g_gain, b_gain;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_R_GAIN, &r_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_G_GAIN, &g_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_B_GAIN, &b_gain);
    if (ret < 0)
        return ret;

    printf("R Gain = %d, G Gain = %d, B Gain = %d\n", r_gain, g_gain, b_gain);

    do {
        printf("R Gain >> ");
        r_gain = get_choose_int();
    } while ((int)r_gain < 0);

    do {
        printf("G Gain >> ");
        g_gain = get_choose_int();
    } while ((int)g_gain < 0);

    do {
        printf("B Gain >> ");
        b_gain = get_choose_int();
    } while ((int)b_gain < 0);

    ret = ioctl(isp_fd, ISP_IOC_SET_R_GAIN, &r_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_R_GAIN fail\n");
        return ret;
    }

    ret = ioctl(isp_fd, ISP_IOC_SET_G_GAIN, &g_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_G_GAIN fail\n");
        return ret;
    }

    ret = ioctl(isp_fd, ISP_IOC_SET_B_GAIN, &b_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_B_GAIN fail\n");
        return ret;
    }

    return 0;
}

static int test_color_gain(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. color gain fomrmat\n");
        printf(" 2. RGB gain\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            test_cg_format();
            break;
        case 2:
            test_rgb_gain();
            break;
        case 99:
            return 0;
        }
    }
}

static int adjust_drc_strength(void)
{
    int drc_str, auto_drc_en;;
    int ret;

    // disable Auto DRC
    auto_drc_en = 0;
    ret = ioctl(isp_fd, AE_IOC_SET_AUTO_DRC, &auto_drc_en);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_DRC_STRENGTH, &drc_str);
    if (ret < 0)
        return ret;
    printf("DRC strength = %d\n", drc_str);

    do {
        printf("DRC strength >> ");
        drc_str = get_choose_int();
    } while (drc_str < 0);

    return ioctl(isp_fd, ISP_IOC_SET_DRC_STRENGTH, &drc_str);
}

static int test_drc(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. DRC strength\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            adjust_drc_strength();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int test_gamma(void)
{
    gamma_lut_t gamma;

    gamma_lut_t bt709 = {
        { 768, 264, 266,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23, 792 },
        {   8,  17,  26,  33,  39,  45,  50,  55,  71,  84,  96, 106, 124, 140, 155, 167,
          179, 191, 201, 211, 220, 230, 238, 247, 248, 249, 250, 251, 252, 253, 254, 255 },
    };

    gamma_lut_t r22 = {
        { 768, 264, 266,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23, 792 },
        {  21,  33,  42,  49,  55,  61,  66,  70,  86,  99, 110, 119, 136, 151, 164, 176,
          187, 197, 207, 216, 224, 232, 240, 247, 248, 249, 250, 251, 252, 253, 254, 255 },
    };
    int adjust_gamma = 0;
    int i, opt, ret = 0;

    // disable auto gamma adjustment
    ret = ioctl(isp_fd, ISP_IOC_SET_ADJUST_GAMMA_EN, &adjust_gamma);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_GAMMA, &gamma);
    if (ret < 0)
        return ret;
    printf("index:");
    for (i=0; i<16; i++)
        printf("0x%x, ", gamma.index[i]);
    printf("\n");

    printf("value:");
    for (i=0; i<16; i++)
        printf("%d, ", gamma.value[i]);
    printf("...\n");

    do {
        printf("Gamma (0:BT709, 1:r2.2 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_GAMMA, &bt709);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_GAMMA, &r22);
    }

    return ret;
}

static int test_cc(void)
{
    int option, i;
    int ret = 0;
    cc_matrix_t cc;
    cc_matrix_t cc_test = {
        { 300,   0,   0,
            0, 256,   0,
            0,   0, 128 },
    };

    printf("----------------------------------------\n");
    printf(" 1. Get Current CC Matrix\n");
    printf(" 2. Set Test CC Matrx\n");
    printf("----------------------------------------\n");
    do {
        printf(">> ");
        option = get_choose_int();
    } while ((option != 1) && (option != 2));

    switch (option) {
    case 1:
        ret = ioctl(isp_fd, ISP_IOC_GET_CC_MATRIX, &cc);
        if (ret < 0)
            return ret;
        for (i=0; i<3; i++)
            printf("%5d, %5d, %5d\n", cc.coe[i*3], cc.coe[i*3+1], cc.coe[i*3+2]);
        break;
    case 2:
        ret = ioctl(isp_fd, ISP_IOC_SET_CC_MATRIX, &cc_test);
        if (ret < 0)
            return ret;
        break;
    }

    return 0;
}

static int test_cv(void)
{
    int option, i;
    int ret = 0;
    cv_matrix_t cv;
    cv_matrix_t cv_test = {
        {  76, 150,   30,
          -44, -84,  128,
          128, -108, -20 },
    };

    printf("----------------------------------------\n");
    printf(" 1. Get Current CV Matrix\n");
    printf(" 2. Set Test CV Matrx\n");
    printf("----------------------------------------\n");
    do {
        printf(">> ");
        option = get_choose_int();
    } while ((option != 1) && (option != 2));

    switch (option) {
    case 1:
        ret = ioctl(isp_fd, ISP_IOC_GET_CV_MATRIX, &cv);
        if (ret < 0)
            return ret;
        for (i=0; i<3; i++)
            printf("%5d, %5d, %5d\n", (int)cv.coe[i*3], (int)cv.coe[i*3+1], (int)cv.coe[i*3+2]);
        break;
    case 2:
        ret = ioctl(isp_fd, ISP_IOC_SET_CV_MATRIX, &cv_test);
        if (ret < 0) {
            printf("ISP_IOC_SET_CV_MATRIX failed\n");
            return ret;
        }
        break;
    }

    return 0;
}

static int adjust_ia_hue(void)
{
    ia_hue_t ia_hue;
    ia_hue_t ia_hue0 = {
        { 0,4,251,0,5,5,4,0 },
    };

    ia_hue_t ia_hue1 = {
        { 0,0,0,0,0,0,0,0 },
    };
    int ret = 0, opt, i;

    ret = ioctl(isp_fd, ISP_IOC_GET_IA_HUE, &ia_hue);
    if (ret < 0)
        return ret;

    printf("IA hue:\n");
    for (i=0; i<8; i++)
        printf("%d ", ia_hue.hue[i]);
    printf("\n");

    do {
        printf("IA Hue (0:IA_HUE0, 1:IA_HUE1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_IA_HUE, &ia_hue0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_IA_HUE, &ia_hue1);
    }

    return ret;
}

static int adjust_ia_saturation(void)
{
    ia_sat_t ia_sat;
    ia_sat_t ia_sat0 = {
        { 166,166,166,134,166,166,143,155 },
    };

    ia_sat_t ia_sat1 = {
        { 128,128,128,128,128,128,128,128 },
    };
    int ret = 0, opt, i;

    ret = ioctl(isp_fd, ISP_IOC_GET_IA_SATURATION, &ia_sat);
    if (ret < 0)
        return ret;

    printf("IA saturation:\n");
    for (i=0; i<8; i++)
        printf("%d ", ia_sat.saturation[i]);
    printf("\n");

    do {
        printf("IA saturation (0:IA_SAT0, 1:IA_SAT1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_IA_SATURATION, &ia_sat0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_IA_SATURATION, &ia_sat1);
    }

    return ret;
}

static int test_ia(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. IA hue\n");
        printf(" 2. IA saturation\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            adjust_ia_hue();
            break;
        case 2:
            adjust_ia_saturation();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int test_sk(void)
{
    printf("Not support now\n");
    return 0;
}

static int adjust_sp_strength(void)
{
    int sp_str, adjust_nr;;
    int ret;

    // disable Auto adjustment
    adjust_nr = 0;
    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_NR_EN, &adjust_nr);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SP_STRENGTH, &sp_str);
    if (ret < 0)
        return ret;
    printf("Sharpen strength = %d\n", sp_str);

    do {
        printf("Sharpen strength >> ");
        sp_str = get_choose_int();
    } while (sp_str < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SP_STRENGTH, &sp_str);
    return 0;
}

static int adjust_sp_clip(void)
{
    sp_clip_t sp_clip;
    sp_clip_t sp_clip0 = {
        127,    // th
        32,     // clip0
        128,    // clip1
        0,      // reduce
    };

    sp_clip_t sp_clip1 = {
        64,     // th
        48,     // clip0
        96,     // clip1
        0,      // reduce
    };
    int ret = 0, opt;

    ret = ioctl(isp_fd, ISP_IOC_GET_SP_CLIP, &sp_clip);
    if (ret < 0)
        return ret;

    printf("th = %d\n",     sp_clip.th);
    printf("clip0 = %d\n",  sp_clip.clip0);
    printf("clip1 = %d\n",  sp_clip.clip1);
    printf("reduce = %d\n", sp_clip.reduce);

    do {
        printf("Sharpen Clip (0:SP_CLIP0, 1:SP_CLIP1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_SP_CLIP, &sp_clip0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_SP_CLIP, &sp_clip1);
    }

    return ret;
}

static int adjust_sp_f_curve(void)
{
    sp_curve_t sp_f;
    sp_curve_t sp_f0 = {
        {0, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38, 39, 40, 41, 41, 42, 43, 44, 45, 46, 47, 48, 49,
         51, 52, 53, 54, 55, 57, 58, 60, 61, 63, 65, 67, 69, 72, 74, 76, 78, 81, 83, 86, 88, 91, 93,
         95, 98, 100, 102, 104, 107, 109, 110, 112, 114, 115, 117, 118, 119, 120, 121, 123, 124, 125,
         126, 127, 128, 129, 130, 131, 133, 134, 134, 135, 136, 137, 138, 139, 140, 140, 141, 141,
         142, 142, 143, 143, 144, 144, 144, 144, 144, 144, 144, 144, 144, 143, 143, 143, 143, 143,
         142, 142, 142, 141, 141, 140, 140, 139, 139, 139, 138, 137, 137, 136, 136, 135, 135, 134,
         133, 133, 132, 132, 131, 130, 130, 129, 128, },
    };

    sp_curve_t sp_f1 = {
        {0, 0, 10, 16, 19, 22, 25, 26, 28, 29, 31, 32, 33, 35, 35, 36, 37, 39, 40, 41, 42, 43, 44, 46,
         47, 48, 50, 51, 53, 54, 56, 57, 59, 61, 63, 65, 68, 70, 72, 74, 77, 79, 82, 84, 86, 88, 90, 93,
         95, 97, 99, 102, 104, 105, 107, 109, 110, 112, 114, 115, 116, 117, 119, 120, 121, 122, 123, 124,
         125, 126, 127, 129, 130, 130, 131, 132, 133, 134, 135, 136, 136, 137, 137, 138, 138, 139, 139,
         140, 140, 140, 140, 140, 140, 140, 140, 141, 140, 140, 140, 140, 140, 139, 139, 139, 138, 138,
         137, 137, 136, 136, 136, 135, 134, 134, 133, 133, 132, 132, 131, 130, 130, 129, 129, 128, 127,
         127, 126, 126},
    };
    int ret = 0, opt, i, j;

    ret = ioctl(isp_fd, ISP_IOC_GET_SP_F_CURVE, &sp_f);
    if (ret < 0)
        return ret;

    printf("sharpen F curve\n");
    for (j=0;j<8;j++){
        for (i=0; i<16; i++)
            printf("%d ", sp_f.F[j*16+i]);
    }
    do {
        printf("Sharpen F curve (0:Curve0, 1:Curve1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_SP_F_CURVE, &sp_f0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_SP_F_CURVE, &sp_f1);
    }

    return ret;
}

static int test_sp(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Sharpness strenght\n");
        printf(" 2. Sharpness clip setting\n");
        printf(" 3. Sharpness F curve\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            adjust_sp_strength();
            break;
        case 2:
            adjust_sp_clip();
            break;
        case 3:
            adjust_sp_f_curve();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int test_cs(void)
{
    cs_reg_t cs_reg;
    cs_reg_t cs_test0 = {
        {12, 64, 90, 135},
        {36, 55, 54},
        {403, 980, 1139, 114},
        36, 18,
    };
    cs_reg_t cs_test1 = {
        {10, 31, 232, 247},
        {36, 184, 168},
        {3788, 865, 1484, 5344},
        36, 36,
    };
    int ret = 0, opt;

    ret = ioctl(isp_fd, ISP_IOC_GET_CS, &cs_reg);
    if (ret < 0)
        return ret;

    printf("in    : %d, %d, %d, %d\n", cs_reg.in[0], cs_reg.in[1], cs_reg.in[2], cs_reg.in[3]);
    printf("out   : %d, %d, %d\n", cs_reg.out[0], cs_reg.out[1], cs_reg.out[2]);
    printf("slope : %d, %d, %d, %d\n", cs_reg.slope[0], cs_reg.slope[1], cs_reg.slope[2], cs_reg.slope[3]);
    printf("cb_th = %d\n", cs_reg.cb_th);
    printf("cr_th = %d\n", cs_reg.cr_th);

    do {
        printf("Chroma suppression (0:CS_TEST0, 1:CS_TEST1 >> ");
        opt = get_choose_int();
    } while (opt < 0);

    if (opt == 0) {
        ret = ioctl(isp_fd, ISP_IOC_SET_CS, &cs_test0);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_CS, &cs_test1);
    }

    return ret;
}

static int test_2d_nr(void)
{
    printf("Not support now\n");
    return 0;
}

static int set_mrnr_en(void)
{
    int mrnr_en;

    do {
        printf("Post Noise Reduction (0:Disable, 1:Enable) >> ");
        mrnr_en = get_choose_int();
    } while (mrnr_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_MRNR_EN, &mrnr_en);
}

static int set_tmnr_en(void)
{
    int tmnr_en;

    do {
        printf("Temporal Noise Reduction (0:Disable, 1:Enable) >> ");
        tmnr_en = get_choose_int();
    } while (tmnr_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_TMNR_EN, &tmnr_en);
}

static int test_3d_nr(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable MRNR\n");
        printf(" 2. Enable/Disable TMNR\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            set_mrnr_en();
            break;
        case 2:
            set_tmnr_en();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

int image_pipe_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Function enable/disable\n");
        printf(" 2. Black level correction\n");
        printf(" 3. Linearity correction\n");
        printf(" 4. Defect pixel correction\n");
        printf(" 5. De-crosstalk\n");
        printf(" 6. Lens uniformity correction\n");
        printf(" 7. ISP digital gain\n");
        printf(" 8. RGB gain\n");
        printf(" 9. Dynamic range correction\n");
        printf("10. Gamma\n");
        printf("11. RGB to RGB\n");
        printf("12. RGB to YUV\n");
        printf("13. Image adjustment\n");
        printf("14. Skin color correction\n");
        printf("15. Sharpness\n");
        printf("16. Chroma suppression\n");
        printf("17. 2D noise reduction\n");
        printf("18. 3D noise reduction\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            test_func_en();
            break;
        case 2:
            test_blc();
            break;
        case 3:
            test_li();
            break;
        case 4:
            test_dpc();
            break;
        case 5:
            test_ctk();
            break;
        case 6:
            test_lsc();
            break;
        case 7:
            test_isp_gain();
            break;
        case 8:
            test_color_gain();
            break;
        case 9:
            test_drc();
            break;
        case 10:
            test_gamma();
            break;
        case 11:
            test_cc();
            break;
        case 12:
            test_cv();
            break;
        case 13:
            test_ia();
            break;
        case 14:
            test_sk();
            break;
        case 15:
            test_sp();
            break;
        case 16:
            test_cs();
            break;
        case 17:
            test_2d_nr();
            break;
        case 18:
            test_3d_nr();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int adjust_brightness(void)
{
    int brightness;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_BRIGHTNESS, &brightness);
    if (ret < 0)
        return ret;
    printf("Current brightness = %d\n", brightness);

    do {
        printf("brightness >> ");
        brightness = get_choose_int();
    } while (brightness < 0);

    return ioctl(isp_fd, ISP_IOC_SET_BRIGHTNESS, &brightness);
}

static int adjust_contrast(void)
{
    int contrast;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_CONTRAST, &contrast);
    if (ret < 0)
        return ret;
    printf("Current contrast = %d\n", contrast);

    do {
        printf("contrast >> ");
        contrast = get_choose_int();
    } while (contrast < 0);

    return ioctl(isp_fd, ISP_IOC_SET_CONTRAST, &contrast);
}

static int adjust_hue(void)
{
    int hue;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_HUE, &hue);
    if (ret < 0)
        return ret;
    printf("Current hue = %d\n", hue);

    do {
        printf("hue >> ");
        hue = get_choose_int();
    } while (hue < 0);

    return ioctl(isp_fd, ISP_IOC_SET_HUE, &hue);
}

static int adjust_saturation(void)
{
    int saturation;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SATURATION, &saturation);
    if (ret < 0)
        return ret;
    printf("Current saturation = %d\n", saturation);

    do {
        printf("saturation >> ");
        saturation = get_choose_int();
    } while (saturation < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SATURATION, &saturation);
}

static int adjust_denoise(void)
{
    int denoise;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_DENOISE, &denoise);
    if (ret < 0)
        return ret;
    printf("Current denoise strength = %d\n", denoise);

    do {
        printf("denoise strength >> ");
        denoise = get_choose_int();
    } while (denoise < 0);

    return ioctl(isp_fd, ISP_IOC_SET_DENOISE, &denoise);
}

static int adjust_shapness(void)
{
    int shapness;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SHARPNESS, &shapness);
    if (ret < 0)
        return ret;
    printf("Current denoise strength = %d\n", shapness);

    do {
        printf("shapness strength >> ");
        shapness = get_choose_int();
    } while (shapness < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SHARPNESS, &shapness);
}

static int adjust_dr_mode(void)
{
    int mode;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_DR_MODE, &mode);
    if (ret < 0)
        return ret;
    printf("Current dynamic range mode = %d\n", mode);

    do {
        printf("dynamic range mode >> ");
        mode = get_choose_int();
    } while (mode < 0);

    return ioctl(isp_fd, ISP_IOC_SET_DR_MODE, &mode);
}

static int set_adjust_nr(void)
{
    int adjust_nr_en;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_NR_EN, &adjust_nr_en);
    if (ret < 0)
        return ret;
    printf("Adjust NR: %s\n", (adjust_nr_en) ? "ENABLE" : "DISABLE");

    do {
        printf("Adjust NR (0:Disable, 1:Enable) >> ");
        adjust_nr_en = get_choose_int();
    } while (adjust_nr_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_ADJUST_NR_EN, &adjust_nr_en);
}

static int get_adj_param(void)
{
    adj_param_t adj_param;
    int i;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_AUTO_ADJ_PARAM, &adj_param);
    if (ret < 0)
        return ret;

    printf("[gain_th] ");
    for (i=0; i<9; i++)
        printf("%d, ", adj_param.gain_th[i]);
    printf("\n");

    printf("[nl_raw] ");
    for (i=0; i<9; i++)
        printf("%d, ", adj_param.nl_raw[i]);
    printf("\n");

    printf("[sp_lvl] ");
    for (i=0; i<9; i++)
        printf("%d, ", adj_param.sp_lvl[i]);
    printf("\n");

    return ret;
}

static int set_adj_param(void)
{
    adj_param_t adj_param = {
        {96,128,256,512,1024,2048,4096,7905,7905},
        {{-169, -169, -169},{-169, -169, -169},{-169, -168, -169},{-170, -167, -170}, {-170, -167, -170},
         {-170, -167, -170},{-170, -167, -170},{-170, -167, -170},{-170, -167, -170}},
        {6144,8192,14336,20480,43008,51200,76288,130560,66638},
        {6144,8192,14336,20480,43008,51200,76288,130560,66638},
        {0,0,0,0,0,0,0,0,0},
        {255,255,64,48,32,20,16,16,16},
        {4,4,4,3,3,3,2,2,2},
        {0,0,0,0,0,0,0,0,0},
        {128,128,128,128,128,128,128,128,128},
        {0,400,450,450,550,750,750,750,750},
        {0,400,450,450,550,750,750,750,750},
        {24,24,22,20,18,18,16,16,16},
        {128,128,128,128,84,64,64,48,48},
        {800,800,800},
        {800,800,800},
        {4,4,4,4,4,5,5,5,5},
    };

    return ioctl(isp_fd, ISP_IOC_SET_AUTO_ADJ_PARAM, &adj_param);
}

static int adjust_nr_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable noise reduction adjustment\n");
        printf(" 2. Get adj_param_t\n");
        printf(" 3. Set adj_param_t\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            set_adjust_nr();
            break;
        case 2:
            get_adj_param();
            break;
        case 3:
            set_adj_param();
            break;
        case 99:
            return 0;
        }
    }
}

static int set_adjust_cc(void)
{
    int adjust_cc_en;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_CC_EN, &adjust_cc_en);
    if (ret < 0)
        return ret;
    printf("Adjust CC: %s\n", (adjust_cc_en) ? "ENABLE" : "DISABLE");

    do {
        printf("Adjust CC (0:Disable, 1:Enable) >> ");
        adjust_cc_en = get_choose_int();
    } while (adjust_cc_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_ADJUST_CC_EN, &adjust_cc_en);
}

static int get_adj_cc_matrix(void)
{
    adj_matrix_t adj_cc;
    int i, j;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_CC_MATRIX, &adj_cc);
    if (ret < 0)
        return ret;
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cc.matrix[j][i]);
        printf("\n");
    }

    return ret;

}

static int set_adj_cc_matrix(void)
{
    adj_matrix_t adj_cc = {
        {{256,0,0,0,250,0,0,0,240},
         {250,0,0,0,240,0,0,0,230},
         {240,0,0,0,230,0,0,0,220}}
    };
    return ioctl(isp_fd, ISP_IOC_SET_ADJUST_CC_MATRIX, &adj_cc);
}

static int get_adj_cv_matrix(void)
{
    adj_matrix_t adj_cv;
    int i, j;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_CV_MATRIX, &adj_cv);
    if (ret < 0)
        return ret;
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cv.matrix[j][i]);
        printf("\n");
    }

    return ret;
}

static int set_adj_cv_matrix(void)
{
    adj_matrix_t adj_cv = {
        {{76,150,30,-44,-84,128,128,-108,-20},
         {76,150,30,-44,-84,128,128,-108,-20},
         {76,150,30,-44,-84,128,128,-108,-20}}
    };
    return ioctl(isp_fd, ISP_IOC_SET_ADJUST_CV_MATRIX, &adj_cv);
}

static int adjust_cc_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable color correction adjustment\n");
        printf(" 2. Get adjust_cc matrix\n");
        printf(" 3. Set adjust_cc matrix\n");
        printf(" 4. Get adjust_cv matrix\n");
        printf(" 5. Set adjust_cv matrix\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            set_adjust_cc();
            break;
        case 2:
            get_adj_cc_matrix();
            break;
        case 3:
            set_adj_cc_matrix();
            break;
        case 4:
            get_adj_cv_matrix();
            break;
        case 5:
            set_adj_cv_matrix();
            break;
        case 99:
            return 0;
        }
    }
}

static int set_auto_contrast(void)
{
    int auto_contrast;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_AUTO_CONTRAST, &auto_contrast);
    if (ret < 0)
        return ret;
    printf("Auto Contrast: %s\n", (auto_contrast) ? "ENABLE" : "DISABLE");

    do {
        printf("Auto Contrast (0:Disable, 1:Enable) >> ");
        auto_contrast = get_choose_int();
    } while (auto_contrast < 0);

    return ioctl(isp_fd, ISP_IOC_SET_AUTO_CONTRAST, &auto_contrast);
}

static int set_gamma_type(void)
{
    int gamma_type;
    int ret;
    char *gamma_name[] = {
        "GM_GAMMA_16",
        "GM_GAMMA_18",
        "GM_GAMMA_20",
        "GM_GAMMA_22",
        "GM_DEF_GAMMA",
        "GM_BT709_GAMMA",
        "GM_SRGB_GAMMA",
        "GM_USER_GAMMA",
    };

    ret = ioctl(isp_fd, ISP_IOC_GET_GAMMA_TYPE, &gamma_type);
    if (ret < 0)
        return ret;
    printf("Current Gamma is : %s\n", gamma_name[gamma_type]);

    do {
        printf("Select gammma type (0~%d) >> ", NUM_OF_GAMMA_TYPE-1);
        gamma_type = get_choose_int();
    } while ((gamma_type < 0) || (gamma_type >= NUM_OF_GAMMA_TYPE));

    return ioctl(isp_fd, ISP_IOC_SET_GAMMA_TYPE, &gamma_type);
}

const u8 gamma_list[3][256] = {
    {0, 1, 5, 10, 16, 22, 29, 35, 40, 45, 48, 52, 55, 58, 61, 64, 66, 69, 71, 73, 76, 78,
     80, 82, 84, 86, 88, 90, 92, 94, 96, 97, 99, 101, 103, 104, 106, 107, 109, 111, 112,
     114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129, 130, 131, 133, 134, 135, 136,
     138, 139, 140, 141, 143, 144, 145, 146, 147, 148, 149, 150, 152, 153, 154, 155, 156,
     157, 158, 159, 160, 161, 162, 163, 163, 164, 165, 166, 167, 168, 169, 170, 171, 171,
     172, 173, 174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 182, 182, 183, 184, 185,
     185, 186, 187, 188, 188, 189, 190, 190, 191, 192, 192, 193, 194, 194, 195, 196, 196,
     197, 198, 198, 199, 199, 200, 201, 201, 202, 202, 203, 204, 204, 205, 205, 206, 206,
     207, 208, 208, 209, 209, 210, 210, 211, 211, 212, 213, 213, 214, 214, 215, 215, 216,
     216, 217, 217, 218, 218, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224, 224,
     224, 225, 225, 226, 226, 227, 227, 228, 228, 229, 229, 229, 230, 230, 231, 231, 232,
     232, 232, 233, 233, 234, 234, 235, 235, 235, 236, 236, 237, 237, 237, 238, 238, 239,
     239, 239, 240, 240, 241, 241, 241, 242, 242, 243, 243, 243, 244, 244, 244, 245, 245,
     246, 246, 246, 247, 247, 247, 248, 248, 248, 249, 249, 250, 250, 250, 251, 251, 251,
     252, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255},

    {0, 0, 2, 4, 7, 10, 14, 18, 22, 27, 32, 38, 43, 48, 53, 57, 61, 64, 67, 70, 73, 76,
     79, 82, 84, 87, 89, 92, 94, 96, 99, 101, 103, 105, 107, 110, 112, 114, 116, 118,
     120, 122, 124, 126, 128, 129, 131, 133, 135, 136, 138, 140, 141, 143, 144, 146, 147,
     149, 150, 152, 153, 154, 156, 157, 158, 159, 160, 162, 163, 164, 165, 166, 167, 168,
     169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 183, 184,
     185, 186, 187, 187, 188, 189, 190, 191, 191, 192, 193, 193, 194, 195, 196, 196, 197,
     198, 198, 199, 200, 200, 201, 201, 202, 203, 203, 204, 204, 205, 206, 206, 207, 207,
     208, 208, 209, 210, 210, 211, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 216,
     217, 217, 218, 218, 219, 219, 220, 220, 221, 221, 221, 222, 222, 223, 223, 224, 224,
     224, 225, 225, 226, 226, 227, 227, 227, 228, 228, 229, 229, 229, 230, 230, 230, 231,
     231, 232, 232, 232, 233, 233, 233, 234, 234, 234, 235, 235, 236, 236, 236, 237, 237,
     237, 238, 238, 238, 239, 239, 239, 240, 240, 240, 240, 241, 241, 241, 242, 242, 242,
     243, 243, 243, 244, 244, 244, 244, 245, 245, 245, 246, 246, 246, 246, 247, 247, 247,
     248, 248, 248, 248, 249, 249, 249, 250, 250, 250, 250, 251, 251, 251, 251, 252, 252,
     252, 252, 253, 253, 253, 254, 254, 254, 254, 255, 255, 255},

    {0, 4, 9, 13, 18, 22, 26, 30, 33, 36, 40, 42, 45, 48, 50, 53, 55, 57, 59, 61, 63, 65,
     67, 69, 71, 73, 75, 76, 78, 80, 81, 83, 84, 86, 87, 89, 90, 92, 93, 95, 96, 97, 99,
     100, 101, 103, 104, 105, 106, 108, 109, 110, 111, 112, 114, 115, 116, 117, 118, 119,
     120, 121, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
     138, 139, 140, 141, 142, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 151, 152,
     153, 154, 155, 156, 156, 157, 158, 159, 160, 161, 161, 162, 163, 164, 165, 165, 166,
     167, 168, 169, 169, 170, 171, 172, 172, 173, 174, 175, 175, 176, 177, 178, 178, 179,
     180, 180, 181, 182, 183, 183, 184, 185, 185, 186, 187, 188, 188, 189, 190, 190, 191,
     192, 192, 193, 194, 194, 195, 196, 196, 197, 198, 198, 199, 200, 200, 201, 201, 202,
     203, 203, 204, 205, 205, 206, 207, 207, 208, 208, 209, 210, 210, 211, 211, 212, 213,
     213, 214, 214, 215, 216, 216, 217, 217, 218, 219, 219, 220, 220, 221, 221, 222, 223,
     223, 224, 224, 225, 225, 226, 227, 227, 228, 228, 229, 229, 230, 231, 231, 232, 232,
     233, 233, 234, 234, 235, 235, 236, 236, 237, 238, 238, 239, 239, 240, 240, 241, 241,
     242, 242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 249, 250, 250,
     251, 251, 252, 252, 253, 253, 254, 254, 255},
};

static int set_gamma_curve(void)
{
    int index;

    do {
        printf("Select gammma (0~2) >> ");
        index = get_choose_int();
    } while ((index < 0) || (index >= 3));

    return ioctl(isp_fd, ISP_IOC_SET_GAMMA_CURVE, gamma_list[index]);
}

static int set_size(void)
{
    int ret = 0, option;
    win_size_t win_size;

    printf("----------------------------------------\n");
    printf(" 1. Get Size\n");
    printf(" 2. Set Size\n");
    printf("----------------------------------------\n");

    do {
        printf(">> ");
        option = get_choose_int();
    } while ((option != 1) && (option != 2));

    switch (option) {
    case 1:
        ret = ioctl(isp_fd, ISP_IOC_GET_SIZE, &win_size);
        if (ret < 0)
            return ret;
        printf("Size=%dx%d\n", win_size.width, win_size.height);
        break;
    case 2:
        do {
            printf("width=");
            win_size.width = get_choose_int();
        } while (win_size.width < 0);
        do {
            printf("height=");
            win_size.height = get_choose_int();
        } while (win_size.height < 0);

        ret = ioctl(isp_fd, ISP_IOC_SET_SIZE, &win_size);
        if (ret < 0)
            return ret;
        break;
    }

    return 0;
}

int user_pref_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Brightness\n");
        printf(" 2. Contrast\n");
        printf(" 3. Hue\n");
        printf(" 4. Saturation\n");
        printf(" 5. Denoise\n");
        printf(" 6. Sharpness\n");
        printf(" 7. Dynamic range mode\n");
        printf(" 8. Noise reduction adjustment\n");
        printf(" 9. Color correction adjustment\n");
        printf("10. Auto contrast\n");
        printf("11. Gamma type\n");
        printf("12. Gamma curve\n");
        printf("13. Size\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            adjust_brightness();
            break;
        case 2:
            adjust_contrast();
            break;
        case 3:
            adjust_hue();
            break;
        case 4:
            adjust_saturation();
            break;
        case 5:
            adjust_denoise();
            break;
        case 6:
            adjust_shapness();
            break;
        case 7:
            adjust_dr_mode();
            break;
        case 8:
            adjust_nr_menu();
            break;
        case 9:
            adjust_cc_menu();
            break;
        case 10:
            set_auto_contrast();
            break;
        case 11:
            set_gamma_type();
            break;
        case 12:
            set_gamma_curve();
            break;
        case 13:
            set_size();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int read_sensor_reg(void)
{
    reg_info_t reg_info;
    u32 addr;
    int ret = 0;

    do {
        printf("addr >> 0x");
        addr = get_choose_hex();
    } while (addr < 0);

    reg_info.addr = addr;
    ret = ioctl(isp_fd, ISP_IOC_READ_SENSOR_REG, &reg_info);
    if (ret > 0)
        printf("ISP_IOC_READ_SENSOR_REG fail\n");
    else
        printf("R[0x%x] = 0x%x\n", reg_info.addr, reg_info.value);
    return ret;
}

static int write_sensor_reg(void)
{
    reg_info_t reg_info;
    u32 addr, val;
    int ret = 0;

    do {
        printf("addr >> 0x");
        addr = get_choose_hex();
    } while (addr < 0);

    reg_info.addr = addr;

    do {
        printf("value >> 0x");
        val = get_choose_hex();
    } while (val < 0);

    reg_info.value = val;
    ret = ioctl(isp_fd, ISP_IOC_WRITE_SENSOR_REG, &reg_info);
    if (ret > 0)
        printf("ISP_IOC_WRITE_SENSOR_REG fail\n");
    else
        printf("R[0x%x] = 0x%x\n", reg_info.addr, reg_info.value);
    return ret;
}

static int adjust_sensor_exp(void)
{
    u32 exp;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_EXP, &exp);
    if (ret < 0)
        return ret;
    printf("Exposure time = %d in unit of 100us\n", exp);

    do {
        printf("Exposure time >> ");
        exp = get_choose_int();
    } while (exp < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_EXP, &exp);
}

static int adjust_sensor_gain(void)
{
    u32 gain;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_GAIN, &gain);
    if (ret < 0)
        return ret;
    printf("Gain = %d\n", gain);

    do {
        printf("Gain >> ");
        gain = get_choose_int();
    } while (gain < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_GAIN, &gain);
}

static int adjust_sensor_fps(void)
{
    int fps;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_FPS, &fps);
    if (ret < 0)
        return ret;
    printf("FPS = %d\n", fps);

    do {
        printf("FPS >> ");
        fps = get_choose_int();
    } while (fps < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_FPS, &fps);
}

static int set_sensor_mirror(void)
{
    int mirror;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MIRROR, &mirror);
    if (ret < 0)
        return ret;
    printf("Mirror: %s\n", (mirror) ? "ENABLE" : "DISABLE");

    do {
        printf("Mirror (0:Disable, 1:Enable) >> ");
        mirror = get_choose_int();
    } while (mirror < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_MIRROR, &mirror);
}

static int set_sensor_flip(void)
{
    int flip;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_FLIP, &flip);
    if (ret < 0)
        return ret;
    printf("Flip: %s\n", (flip) ? "ENABLE" : "DISABLE");

    do {
        printf("Flip (0:Disable, 1:Enable) >> ");
        flip = get_choose_int();
    } while (flip < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_FLIP, &flip);
}

static int set_sensor_ae(void)
{
    int sensor_ae_en;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_AE_EN, &sensor_ae_en);
    if (ret < 0)
        return ret;
    printf("Built-in AE: %s\n", (sensor_ae_en) ? "ENABLE" : "DISABLE");

    do {
        printf("Built-in AE (0:Disable, 1:Enable) >> ");
        sensor_ae_en = get_choose_int();
    } while (sensor_ae_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_AE_EN, &sensor_ae_en);
}

static int set_sensor_awb(void)
{
    int sensor_awb_en;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_AWB_EN, &sensor_awb_en);
    if (ret < 0)
        return ret;
    printf("Built-in AWB: %s\n", (sensor_awb_en) ? "ENABLE" : "DISABLE");

    do {
        printf("Built-in AWB (0:Disable, 1:Enable) >> ");
        sensor_awb_en = get_choose_int();
    } while (sensor_awb_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_AWB_EN, &sensor_awb_en);
}

static int set_sensor_size(void)
{
    int ret = 0, option;
    win_size_t win_size;

    printf("----------------------------------------\n");
    printf(" 1. Get Size\n");
    printf(" 2. Set Size\n");
    printf("----------------------------------------\n");

    do {
        printf(">> ");
        option = get_choose_int();
    } while ((option != 1) && (option != 2));

    switch (option) {
    case 1:
        ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_SIZE, &win_size);
        if (ret < 0)
            return ret;
        printf("Size=%dx%d\n", win_size.width, win_size.height);
        break;
    case 2:
        do {
            printf("width=");
            win_size.width = get_choose_int();
        } while (win_size.width < 0);
        do {
            printf("height=");
            win_size.height = get_choose_int();
        } while (win_size.height < 0);

        ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_SIZE, &win_size);
        if (ret < 0)
            return ret;
        break;
    }

    return 0;
}

static int adjust_sensor_a_gain(void)
{
    u32 gain, min_gain, max_gain;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MIN_A_GAIN, &min_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MAX_A_GAIN, &max_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_A_GAIN, &gain);
    if (ret < 0)
        return ret;
    printf("Sensor Analog Gain = %d\n", gain);

    do {
        printf("Sensor Analog Gain (%d ~ %d) >> ", min_gain, max_gain);
        gain = get_choose_int();
    } while (gain < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_A_GAIN, &gain);
}

static int adjust_sensor_d_gain(void)
{
    u32 gain, min_gain, max_gain;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MIN_D_GAIN, &min_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MAX_D_GAIN, &max_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_D_GAIN, &gain);
    if (ret < 0)
        return ret;
    printf("Sensor Digital Gain = %d\n", gain);

    do {
        printf("Sensor Digital Gain (%d ~ %d) >> ", min_gain, max_gain);
        gain = get_choose_int();
    } while (gain < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_D_GAIN, &gain);
}

int sensor_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1.  Read sensor register\n");
        printf(" 2.  Write sensor register\n");
        printf(" 3.  Sensor exposure time\n");
        printf(" 4.  Sensor gain\n");
        printf(" 5.  Sensor FPS\n");
        printf(" 6.  Sensor Mirror\n");
        printf(" 7.  Sensor Flip\n");
        printf(" 8.  Sensor AE\n");
        printf(" 9.  Sensor AWB\n");
        printf(" 10. Sensor Size\n");
        printf(" 11. Sensor analog gain\n");
        printf(" 12. Sensor digital gain\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            read_sensor_reg();
            break;
        case 2:
            write_sensor_reg();
            break;
        case 3:
            adjust_sensor_exp();
            break;
        case 4:
            adjust_sensor_gain();
            break;
        case 5:
            adjust_sensor_fps();
            break;
        case 6:
            set_sensor_mirror();
            break;
        case 7:
            set_sensor_flip();
            break;
        case 8:
            set_sensor_ae();
            break;
        case 9:
            set_sensor_awb();
            break;
        case 10:
            set_sensor_size();
            break;
        case 11:
            adjust_sensor_a_gain();
            break;
        case 12:
            adjust_sensor_d_gain();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int ae_set_enable(void)
{
    int ae_en;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_ENABLE, &ae_en);
    if (ret < 0)
        return ret;
    printf("AE: %s\n", (ae_en) ? "ENABLE" : "DISABLE");

    do {
        printf("AE (0:Disable, 1:Enable) >> ");
        ae_en = get_choose_int();
    } while (ae_en < 0);

    return ioctl(isp_fd, AE_IOC_SET_ENABLE, &ae_en);
}

static int ae_get_current_y(void)
{
    int current_y;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_CURRENTY, &current_y);
    if (ret < 0)
        return ret;
    printf("Current Y = %d\n", current_y);

    return 0;
}

static int ae_adjust_target_y(void)
{
    int target_y;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_TARGETY, &target_y);
    if (ret < 0)
        return ret;
    printf("Target Y = %d\n", target_y);

    do {
        printf("Target Y >> ");
        target_y = get_choose_int();
    } while (target_y < 0);

    return ioctl(isp_fd, AE_IOC_SET_TARGETY, &target_y);
}

static int ae_adjust_converge_th(void)
{
    u32 th;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_CONVERGE_TH, &th);
    if (ret < 0)
        return ret;
    printf("Converge threshold = %d\n", th);

    do {
        printf("Converge threshold >> ");
        th = get_choose_int();
    } while (th < 0);

    return ioctl(isp_fd, AE_IOC_SET_CONVERGE_TH, &th);
}

static int ae_adjust_speed(void)
{
    int speed;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_SPEED, &speed);
    if (ret < 0)
        return ret;
    printf("Converge speed = %d\n", speed);

    do {
        printf("Converge speed >> ");
        speed = get_choose_int();
    } while (speed < 0);

    return ioctl(isp_fd, AE_IOC_SET_SPEED, &speed);
}

static int ae_adjust_pwr_freq(void)
{
    int mode;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_PWR_FREQ, &mode);
    if (ret < 0)
        return ret;
    printf("Anti-Flicker Mode: %s\n", (mode == AE_PWR_FREQ_60) ? "60Hz" : "50Hz");

    do {
        printf("Anti-Flicker Mode (0:60Hz, 1:50Hz) >> ");
        mode = get_choose_int();
    } while (mode < 0);

    return ioctl(isp_fd, AE_IOC_SET_PWR_FREQ, &mode);
}

static int ae_adjust_min_exp(void)
{
    u32 exp;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_MIN_EXP, &exp);
    if (ret < 0)
        return ret;
    printf("Minimum exposure time = %d in unit of 100us\n", exp);

    do {
        printf("Minimum exposure time >> ");
        exp = get_choose_int();
    } while (exp < 0);

    return ioctl(isp_fd, AE_IOC_SET_MIN_EXP, &exp);
}

static int ae_adjust_max_exp(void)
{
    u32 exp;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_MAX_EXP, &exp);
    if (ret < 0)
        return ret;
    printf("Maximum exposure time = %d in unit of 100us\n", exp);

    do {
        printf("Maximum exposure time >> ");
        exp = get_choose_int();
    } while (exp < 0);

    return ioctl(isp_fd, AE_IOC_SET_MAX_EXP, &exp);
}

static int ae_adjust_min_gain(void)
{
    u32 gain;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_MIN_GAIN, &gain);
    if (ret < 0)
        return ret;
    printf("Minimum gain = %d\n", gain);

    do {
        printf("Minimum gain >> ");
        gain = get_choose_int();
    } while (gain < 0);

    return ioctl(isp_fd, AE_IOC_SET_MIN_EXP, &gain);
}

static int ae_adjust_max_gain(void)
{
    u32 gain;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_MAX_GAIN, &gain);
    if (ret < 0)
        return ret;
    printf("Maximum gain = %d\n", gain);

    do {
        printf("Maximum gain >> ");
        gain = get_choose_int();
    } while (gain < 0);

    return ioctl(isp_fd, AE_IOC_SET_MAX_GAIN, &gain);
}

static int ae_adjust_win_weight(void)
{
    int wei[9];
    int ret;
    int i;
    int wei_set[3][9] =
        {{1,1,1,2,4,2,2,2,2},
         {0,0,0,0,1,0,0,0,0},
         {1,1,1,1,4,1,1,1,1}};
    ret = ioctl(isp_fd, AE_IOC_GET_WIN_WEIGHT, &wei[0]);
    if (ret < 0)
        return ret;
    printf("Current weighting\n");
    printf("[%2d, %2d, %2d]\n", wei[0], wei[1], wei[2]);
    printf("[%2d, %2d, %2d]\n", wei[3], wei[4], wei[5]);
    printf("[%2d, %2d, %2d]\n", wei[6], wei[7], wei[8]);

    printf("Please select weight setting\n");
    printf("0: 1,1,1,2,4,2,2,2,2\n");
    printf("1: 0,0,0,0,1,0,0,0,0\n");
    printf("2: 1,1,1,1,4,1,1,1,1\n");
    do {
        printf("Select >> ");
        i = get_choose_int();
    } while (i < 0 || i > 2);

    return ioctl(isp_fd, AE_IOC_SET_WIN_WEIGHT, &wei_set[i][0]);

}

static int ae_get_ev_value(void)
{
    u32 ev_value;
    int ret;

    ret = ioctl(isp_fd, AE_IOC_GET_EV_VALUE, &ev_value);
    if (ret < 0)
        return ret;
    printf("Current EV = %d\n", ev_value);

    return 0;
}

int ae_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable AE\n");
        printf(" 2. Get current Y\n");
        printf(" 3. Target Y\n");
        printf(" 4. Converge threshold\n");
        printf(" 5. Converge speed\n");
        printf(" 6. Anti-Flicker mode\n");
        printf(" 7. Min exposure time\n");
        printf(" 8. Max exposure time\n");
        printf(" 9. Min gain\n");
        printf("10. Max gain\n");
        printf("11. Window Weighting\n");
        printf("12. Get EV value\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            ae_set_enable();
            break;
        case 2:
            ae_get_current_y();
            break;
        case 3:
            ae_adjust_target_y();
            break;
        case 4:
            ae_adjust_converge_th();
            break;
        case 5:
            ae_adjust_speed();
            break;
        case 6:
            ae_adjust_pwr_freq();
            break;
        case 7:
            ae_adjust_min_exp();
            break;
        case 8:
            ae_adjust_max_exp();
            break;
        case 9:
            ae_adjust_min_gain();
            break;
        case 10:
            ae_adjust_max_gain();
            break;
        case 11:
            ae_adjust_win_weight();
            break;
        case 12:
            ae_get_ev_value();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int awb_set_enable(void)
{
    int awb_en;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_ENABLE, &awb_en);
    if (ret < 0)
        return ret;
    printf("AWB: %s\n", (awb_en) ? "ENABLE" : "DISABLE");

    do {
        printf("AWB (0:Disable, 1:Enable) >> ");
        awb_en = get_choose_int();
    } while (awb_en < 0);

    return ioctl(isp_fd, AWB_IOC_SET_ENABLE, &awb_en);
}

static int awb_get_rg_ratio(void)
{
    u32 rg_ratio;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_RG_RATIO, &rg_ratio);
    if (ret < 0)
        return ret;
    printf("Current R/G = %d\n", rg_ratio);

    return 0;
}

static int awb_get_bg_ratio(void)
{
    u32 bg_ratio;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_BG_RATIO, &bg_ratio);
    if (ret < 0)
        return ret;
    printf("Current B/G = %d\n", bg_ratio);

    return 0;
}

static int awb_get_rb_ratio(void)
{
    u32 rb_ratio;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_RB_RATIO, &rb_ratio);
    if (ret < 0)
        return ret;
    printf("Current R/B = %d\n", rb_ratio);

    return 0;
}

static int awb_adjust_converge_th(void)
{
    u32 th;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_CONVERGE_TH, &th);
    if (ret < 0)
        return ret;
    printf("Converge threshold = %d\n", th);

    do {
        printf("Converge threshold (UFIX 22.10) >> ");
        th = get_choose_int();
    } while (th < 0);

    return ioctl(isp_fd, AWB_IOC_SET_CONVERGE_TH, &th);
}

static int awb_adjust_speed(void)
{
    int speed;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_SPEED, &speed);
    if (ret < 0)
        return ret;
    printf("Converge speed = %d\n", speed);

    do {
        printf("Converge speed >> ");
        speed = get_choose_int();
    } while (speed < 0);

    return ioctl(isp_fd, AWB_IOC_SET_SPEED, &speed);
}

static int awb_adjust_freeze_cnt(void)
{
    int freeze_cnt;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_FREEZE_SEG, &freeze_cnt);
    if (ret < 0)
        return ret;
    printf("Freeze condition = %d\n", freeze_cnt);

    do {
        printf("Freeze condition >> ");
        freeze_cnt = get_choose_int();
    } while (freeze_cnt < 0);

    return ioctl(isp_fd, AWB_IOC_SET_FREEZE_SEG, &freeze_cnt);
}

static int awb_adjust_target_rg_ratio(void)
{
    awb_target_t rg_target;
    int rg_ratio;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_RG_TARGET, &rg_target);
    if (ret < 0)
        return ret;
    printf("D65 Target R/G = %d\n", rg_target.target_ratio[0]);
    printf("CWF Target R/G = %d\n", rg_target.target_ratio[1]);
    printf("A   Target R/G = %d\n", rg_target.target_ratio[2]);

    do {
        printf("D65 Target R/G (UFIX 22.10) >> ");
        rg_ratio = get_choose_int();
    } while (rg_ratio < 0);
    rg_target.target_ratio[0] = rg_ratio;

    do {
        printf("CWF Target R/G (UFIX 22.10) >> ");
        rg_ratio = get_choose_int();
    } while (rg_ratio < 0);
    rg_target.target_ratio[1] = rg_ratio;

    do {
        printf("A   Target R/G (UFIX 22.10) >> ");
        rg_ratio = get_choose_int();
    } while (rg_ratio < 0);
    rg_target.target_ratio[2] = rg_ratio;

    return ioctl(isp_fd, AWB_IOC_SET_RG_TARGET, &rg_target);
}

static int awb_adjust_target_bg_ratio(void)
{
    awb_target_t bg_target;
    int bg_ratio;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_BG_TARGET, &bg_target);
    if (ret < 0)
        return ret;
    printf("Target B/G = %d, %d, %d\n", bg_target.target_ratio[0],
            bg_target.target_ratio[1], bg_target.target_ratio[2]);

    do {
        printf("D65 Target B/G (UFIX 22.10) >> ");
        bg_ratio = get_choose_int();
    } while (bg_ratio < 0);
    bg_target.target_ratio[0] = bg_ratio;

    do {
        printf("CWF Target B/G (UFIX 22.10) >> ");
        bg_ratio = get_choose_int();
    } while (bg_ratio < 0);
    bg_target.target_ratio[1] = bg_ratio;

    do {
        printf("A   Target B/G (UFIX 22.10) >> ");
        bg_ratio = get_choose_int();
    } while (bg_ratio < 0);
    bg_target.target_ratio[2] = bg_ratio;

    return ioctl(isp_fd, AWB_IOC_SET_BG_TARGET, &bg_target);
}

static int awb_get_rgb_gain(void)
{
    u32 r_gain, g_gain, b_gain;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_R_GAIN, &r_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_G_GAIN, &g_gain);
    if (ret < 0)
        return ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_B_GAIN, &b_gain);
    if (ret < 0)
        return ret;

    printf("R Gain = %d, G Gain = %d, B Gain = %d\n", r_gain, g_gain, b_gain);
    return ret;
}

static int awb_set_rgb_gain(void)
{
    u32 r_gain, g_gain, b_gain;
    int ret = 0;

    do {
        printf("R Gain (UFIX 2.7) >> ");
        r_gain = get_choose_int();
    } while ((int)r_gain < 0);

    do {
        printf("G Gain (UFIX 2.7) >> ");
        g_gain = get_choose_int();
    } while ((int)g_gain < 0);

    do {
        printf("B Gain (UFIX 2.7) >> ");
        b_gain = get_choose_int();
    } while ((int)b_gain < 0);

    ret = ioctl(isp_fd, ISP_IOC_SET_R_GAIN, &r_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_R_GAIN fail\n");
        return ret;
    }

    ret = ioctl(isp_fd, ISP_IOC_SET_G_GAIN, &g_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_G_GAIN fail\n");
        return ret;
    }

    ret = ioctl(isp_fd, ISP_IOC_SET_B_GAIN, &b_gain);
    if (ret < 0) {
        printf("ISP_IOC_SET_B_GAIN fail\n");
        return ret;
    }

    return ret;
}

static int awb_get_scene_mode(void)
{
    int scene_mode;
    int ret = 0;

    ret = ioctl(isp_fd, AWB_IOC_GET_SCENE_MODE, &scene_mode);
    if (ret < 0) {
        printf("AWB_IOC_GET_SCENE_MODE fail\n");
        return ret;
    }
    printf("Scene Mode = %d\n", scene_mode);

    return ret;
}

static int awb_set_scene_mode(void)
{
    int scene_mode;
    int ret = 0;

    do {
        printf("Scene Mode (0~5) >> ");
        scene_mode = get_choose_int();
    } while (scene_mode < 0 || scene_mode > 5);

    ret = ioctl(isp_fd, AWB_IOC_SET_SCENE_MODE, &scene_mode);
    if (ret < 0) {
        printf("AWB_IOC_SET_SCENE_MODE fail\n");
        return ret;
    }

    return ret;
}

int awb_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable AWB\n");
        printf(" 2. R/G ratio\n");
        printf(" 3. B/G ratio\n");
        printf(" 4. R/B ratio\n");
        printf(" 5. Converge threshold\n");
        printf(" 6. Converge speed\n");
        printf(" 7. Freeze condition\n");
        printf(" 8. Target R/G ratio\n");
        printf(" 9. Target B/G ratio\n");
        printf("10. Get RGB Gain\n");
        printf("11. Set RGB Gain\n");
        printf("12. Get Scene Mode\n");
        printf("13. Set Scene Mode\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            awb_set_enable();
            break;
        case 2:
            awb_get_rg_ratio();
            break;
        case 3:
            awb_get_bg_ratio();
            break;
        case 4:
            awb_get_rb_ratio();
            break;
        case 5:
            awb_adjust_converge_th();
            break;
        case 6:
            awb_adjust_speed();
            break;
        case 7:
            awb_adjust_freeze_cnt();
            break;
        case 8:
            awb_adjust_target_rg_ratio();
            break;
        case 9:
            awb_adjust_target_bg_ratio();
            break;
        case 10:
            awb_get_rgb_gain();
            break;
        case 11:
            awb_set_rgb_gain();
            break;
        case 12:
            awb_get_scene_mode();
            break;
        case 13:
            awb_set_scene_mode();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int af_set_enable(void)
{
    int af_en;
    int ret;

    ret = ioctl(isp_fd, AF_IOC_GET_ENABLE, &af_en);
    if (ret < 0)
        return ret;
    printf("AF: %s\n", (af_en) ? "ENABLE" : "DISABLE");

    do {
        printf("AF (0:Disable, 1:Enable) >> ");
        af_en = get_choose_int();
    } while (af_en < 0);

    return ioctl(isp_fd, AF_IOC_SET_ENABLE, &af_en);
}

static int af_set_mode(void)
{
    int af_mode;
    int ret;

    ret = ioctl(isp_fd, AF_IOC_GET_SHOT_MODE, &af_mode);
    if (ret < 0)
        return ret;
    printf("AF mode: %s\n", (af_mode) ? "One Shot" : "Continous");

    do {
        printf("AF mode (0:One Shot, 1:Continous) >> ");
        af_mode = get_choose_int();
    } while (af_mode < 0);

    return ioctl(isp_fd, AF_IOC_SET_SHOT_MODE, &af_mode);
}

static int af_re_trigger(void)
{
    int ret=0;
    int busy=1;
    int timeout=0;
    ret = ioctl(isp_fd, AF_IOC_SET_RE_TRIGGER, NULL);
    if (ret < 0)
        return ret;
    printf("AF Re-Trigger\n");
    printf("Wait for converge ...\n");
    do {
        printf(".");
        ioctl(isp_fd, AF_IOC_CHECK_BUSY, &busy);
        sleep(1);
    } while (busy == 1 && ++timeout < 10);
    if(timeout >= 10)
        printf("\nAF timeout !\n");
    else
        printf("\nAF converged\n");
    return 0;
}

static int show_fv(void)
{
    int ret=0;
    int tmp_int;
    af_sta_t af_sta;
    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if((ret = ioctl(isp_fd, ISP_IOC_GET_AF_STA, &af_sta)) < 0){
        printf("ISP_IOC_GET_AF_STA fail!");
        return ret;
    }
    printf("Focus Value\n");
    printf("[%12d][%12d][%12d]\n", af_sta.af_result_w[0], af_sta.af_result_w[1], af_sta.af_result_w[2]);
    printf("[%12d][%12d][%12d]\n", af_sta.af_result_w[3], af_sta.af_result_w[4], af_sta.af_result_w[5]);
    printf("[%12d][%12d][%12d]\n", af_sta.af_result_w[6], af_sta.af_result_w[7], af_sta.af_result_w[8]);

    return ret;
}

int af_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable/Disable AF\n");
        printf(" 2. One Shot/Continous mode\n");
        printf(" 3. Re-Trigger\n");
        printf(" 4. Get FV\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            af_set_enable();
            break;
        case 2:
            af_set_mode();
            break;
        case 3:
            af_re_trigger();
            break;
        case 4:
            show_fv();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int get_hist_sta_cfg(void)
{
    hist_sta_cfg_t hist_sta_cfg;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_HIST_CFG, &hist_sta_cfg)) < 0){
        printf("ISP_IOC_GET_HIST_CFG fail!");
        return ret;
    }

    printf("hist_sta_cfg:\n");
    printf("    src    = %d\n", hist_sta_cfg.src);
    printf("    chsel  = %d\n", hist_sta_cfg.chsel);
    printf("    mode   = %d\n", hist_sta_cfg.mode);
    printf("    shrink = %d\n", hist_sta_cfg.shrink);
    return 0;
}

static int set_hist_sta_cfg(void)
{
    hist_sta_cfg_t hist_sta_cfg;
    int src, mode, chsel, shrink;

    do {
        printf("src (0:AFTER_GG, 1:AFTER_CI, 2:AFTER_CC, 3:AFTER_CS) >> ");
        src = get_choose_int();
    } while (src < 0 || src > 3);

    do {
        printf("chsel (0:Y, 1:G, 2:B/Cb, 3:R/Cr) >> ");
        chsel = get_choose_int();
    } while (chsel < 0 || chsel > 3);

    do {
        printf("mode (0:RGB_DIV3, 1:R2GB_DIV4) >> ");
        mode = get_choose_int();
    } while (mode < 0 || mode > 1);

    do {
        printf("shrink (0~7) >> ");
        shrink = get_choose_int();
    } while (shrink < 0 || shrink > 7);

    hist_sta_cfg.src = src;
    hist_sta_cfg.chsel = chsel;
    hist_sta_cfg.mode = mode;
    hist_sta_cfg.shrink = shrink;

    return ioctl(isp_fd, ISP_IOC_SET_HIST_CFG, &hist_sta_cfg);
}

static int get_hist_win_cfg(void)
{
    hist_win_cfg_t hist_win_cfg;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_HIST_WIN, &hist_win_cfg)) < 0){
        printf("ISP_IOC_GET_HIST_WIN fail!");
        return ret;
    }

    printf("window start(%d, %d), width=%d, height=%d\n",
        hist_win_cfg.hist_win.x, hist_win_cfg.hist_win.y,
        hist_win_cfg.hist_win.width, hist_win_cfg.hist_win.height);

    return 0;
}

static int set_hist_win_cfg(void)
{
    hist_win_cfg_t hist_win_cfg;
    win_rect_t act_win;
    int x, y, w, h;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_ACT_WIN, &act_win)) < 0){
        printf("ISP_IOC_GET_ACT_WIN fail!");
        return ret;
    }

    do {
        printf("x (0~%d) >> ", act_win.width - 1);
        x = get_choose_int();
    } while (x < 0 || x >= act_win.width);

    do {
        printf("y (0~%d) >> ", act_win.height - 1);
        y = get_choose_int();
    } while (y < 0 || y >= act_win.height);

    do {
        printf("width (0~%d)>> ", act_win.width - x);
        w = get_choose_int();
    } while (w < 0 || w > act_win.width - x);

    do {
        printf("height (0~%d)>> ", act_win.height - y);
        h = get_choose_int();
    } while (h < 0 || h > act_win.height - y);

    hist_win_cfg.hist_win.x = x;
    hist_win_cfg.hist_win.y = y;
    hist_win_cfg.hist_win.width = w;
    hist_win_cfg.hist_win.height = h;

    return ioctl(isp_fd, ISP_IOC_SET_HIST_WIN, &hist_win_cfg);
}

static int get_hist_pre_gamma(void)
{
    hist_pre_gamma_t hist_pre_gamma;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_HIST_PRE_GAMMA, &hist_pre_gamma)) < 0){
        printf("ISP_IOC_GET_HIST_PRE_GAMMA fail!");
        return ret;
    }

    for (i=0; i<6; i++) {
        printf("in[%d]=%5d    out[%d]=%5d    slope[%d]=%5d\n",
            i, hist_pre_gamma.in[i], i, hist_pre_gamma.out[i], i, hist_pre_gamma.slope[i]);
    }
    printf("                               slope[%d]=%5d\n", i, hist_pre_gamma.slope[6]);
    return 0;
}

static int set_hist_pre_gamma(void)
{
    hist_pre_gamma_t * ppre_gamma;
    hist_pre_gamma_t gamma_raw12={
        {4, 8, 16, 32, 64, 128}, //in
        {4, 8, 16, 32, 64, 128},
        {256 ,256, 256, 256, 256, 256, 256}
    };
    hist_pre_gamma_t gamma_raw16={
        {4, 8, 16, 32, 64, 255}, //in
        {4*16, 8*16, 16*16, 32*16, 64*16, 255*16},
        {4096 ,4096, 4096, 4096, 4096, 4096, 0}
    };
    int option;

    do {
        printf("histogram pre-gamma (0:RAW12, 1:RAW16)>> ");
        option = get_choose_int();
    } while (option < 0 || option > 1);

    ppre_gamma = (option == 0) ? &gamma_raw12 : &gamma_raw16;
    return ioctl(isp_fd, ISP_IOC_SET_HIST_PRE_GAMMA, ppre_gamma);
}

static int get_histogram(void)
{
    hist_sta_t hist_sta;
    int tmp_int, i, j;
    int ret;

WAIT_HIST_READY:
    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if ((ret = ioctl(isp_fd, ISP_IOC_GET_HISTOGRAM, &hist_sta)) < 0){
        printf("ISP_IOC_GET_HISTOGRAM fail!");
        return ret;
    }
    if (hist_sta.tss_hist_idx != 0)
        goto WAIT_HIST_READY;

    for (j=0; j<32; j++) {
        for (i=0; i<8; i++)
            printf("[%3d] %5d  ", j*8+i, hist_sta.y_bin[j*8+i]);
        printf("\n");
    }

    return 0;
}

int hist_statistic_menu(void)
{
    int option, ae_en;

    // disable Grain-Media AE
    ae_en = 0;
    ioctl(isp_fd, AE_IOC_SET_ENABLE, &ae_en);
    printf("Disable Grain-Media AE\n");

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Get histogram config\n");
        printf(" 2. Set histogram config\n");
        printf(" 3. Get histogram window\n");
        printf(" 4. Set histogram window\n");
        printf(" 5. Get histogram pre-gamma\n");
        printf(" 6. Set histogram pre-gamma\n");
        printf(" 7. Get histogram\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            get_hist_sta_cfg();
            break;
        case 2:
            set_hist_sta_cfg();
            break;
        case 3:
            get_hist_win_cfg();
            break;
        case 4:
            set_hist_win_cfg();
            break;
        case 5:
            get_hist_pre_gamma();
            break;
        case 6:
            set_hist_pre_gamma();
            break;
        case 7:
            get_histogram();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int get_ae_sta_cfg(void)
{
    ae_sta_cfg_t ae_sta_cfg;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AE_CFG, &ae_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AE_CFG fail!");
        return ret;
    }

    printf("ae_sta_cfg:\n");
    printf("    src     = %d\n",   ae_sta_cfg.src);
    printf("    bt      = %d\n",   ae_sta_cfg.bt);
    printf("    vbt     = %d\n",   ae_sta_cfg.vbt);
    printf("    wt      = %d\n",   ae_sta_cfg.wt);
    printf("    vwt     = %d\n",   ae_sta_cfg.vwt);
    printf("    y_mode  = %d\n",   ae_sta_cfg.y_mode);
    printf("    wins_en = 0x%x\n", ae_sta_cfg.wins_en);
    return 0;
}

static int set_ae_sta_cfg(void)
{
    ae_sta_cfg_t ae_sta_cfg;
    int src, y_mode, wins_en;

    do {
        printf("src (0:AFTER_GG, 1:BEFORE_GG) >> ");
        src = get_choose_int();
    } while (src < 0 || src > 1);

    do {
        printf("y_mode (0:RGrGbB_DIV4, 1:GrGb_DIV2) >> ");
        y_mode = get_choose_int();
    } while (y_mode < 0 || y_mode > 1);

    do {
        printf("wins_en (0~0x1FF) >> 0x");
        wins_en = get_choose_hex();
    } while (wins_en < 0 || wins_en > 0x1FF);

    ae_sta_cfg.src = src;
    ae_sta_cfg.bt = 0;
    ae_sta_cfg.vbt = 0;
    ae_sta_cfg.wt = 65535;
    ae_sta_cfg.vwt = 65535;
    ae_sta_cfg.y_mode = y_mode;
    ae_sta_cfg.wins_en = wins_en;

    return ioctl(isp_fd, ISP_IOC_SET_AE_CFG, &ae_sta_cfg);
}

static int get_ae_win_cfg(void)
{
    ae_win_cfg_t ae_win_cfg;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AE_WIN, &ae_win_cfg)) < 0){
        printf("ISP_IOC_GET_AE_WIN fail!");
        return ret;
    }

    for (i=0; i<9; i++) {
        printf("ae_win[%d] start(%d, %d), width=%d, height=%d\n", i,
            ae_win_cfg.ae_win[i].x, ae_win_cfg.ae_win[i].y,
            ae_win_cfg.ae_win[i].width, ae_win_cfg.ae_win[i].height);
    }

    return 0;
}

static int set_ae_win_cfg(void)
{
    ae_win_cfg_t ae_win_cfg;
    win_rect_t act_win;
    int win_idx, x, y, w, h;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_ACT_WIN, &act_win)) < 0){
        printf("ISP_IOC_GET_ACT_WIN fail!");
        return ret;
    }

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AE_WIN, &ae_win_cfg)) < 0){
        printf("ISP_IOC_GET_AE_WIN fail!");
        return ret;
    }

    do {
        printf("window (0~8) >> ");
        win_idx = get_choose_int();
    } while (win_idx < 0 || win_idx > 8);

    do {
        printf("x (0~%d) >> ", act_win.width - 1);
        x = get_choose_int();
    } while (x < 0 || x >= act_win.width);

    do {
        printf("y (0~%d) >> ", act_win.height - 1);
        y = get_choose_int();
    } while (y < 0 || y >= act_win.height);

    do {
        printf("width (0~%d)>> ", act_win.width - x);
        w = get_choose_int();
    } while (w < 0 || w > act_win.width - x);

    do {
        printf("height (0~%d)>> ", act_win.height - y);
        h = get_choose_int();
    } while (h < 0 || h > act_win.height - y);

    ae_win_cfg.ae_win[win_idx].x = x;
    ae_win_cfg.ae_win[win_idx].y = y;
    ae_win_cfg.ae_win[win_idx].width = w;
    ae_win_cfg.ae_win[win_idx].height = h;

    return ioctl(isp_fd, ISP_IOC_SET_AE_WIN, &ae_win_cfg);
}

static int get_ae_pre_gamma(void)
{
    ae_pre_gamma_t ae_pre_gamma;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AE_PRE_GAMMA, &ae_pre_gamma)) < 0){
        printf("ISP_IOC_GET_AE_PRE_GAMMA fail!");
        return ret;
    }

    for (i=0; i<6; i++) {
        printf("in[%d]=%5d    out[%d]=%5d    slope[%d]=%5d\n",
            i, ae_pre_gamma.in[i], i, ae_pre_gamma.out[i], i, ae_pre_gamma.slope[i]);
    }
    printf("                               slope[%d]=%5d\n", i, ae_pre_gamma.slope[6]);
    return 0;
}

static int set_ae_pre_gamma(void)
{
    ae_pre_gamma_t * ppre_gamma;

    ae_pre_gamma_t li_gamma = {
        {4, 8, 16, 32, 64, 128}, //in
        {34, 50, 71, 100, 138, 187}, //out
        {2186, 1023, 683, 454, 304, 198, 136} //slope
    };
    ae_pre_gamma_t wdr_gamma = {
        {4, 8, 16, 32, 64, 255}, //in
        {34, 50, 71, 100, 138, 255}, //out
        {2186, 1023, 683, 454, 304, 157, 0} //slope
    };
    int option;

    do {
        printf("AE pre-gamma (0:Linear, 1:WDR)>> ");
        option = get_choose_int();
    } while (option < 0 || option > 1);

    ppre_gamma = (option == 0) ? &li_gamma : &wdr_gamma;
    return ioctl(isp_fd, ISP_IOC_SET_AE_PRE_GAMMA, ppre_gamma);
}

static int get_ae_sta(void)
{
    ae_sta_t ae_sta;
    unsigned long long sum[3];
    int tmp_int, i;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AE_STA, &ae_sta)) < 0){
        printf("ISP_IOC_GET_AE_STA fail!");
        return ret;
    }

    for (i=0; i<9; i++) {
        sum[0] = ((unsigned long long)ae_sta.win_sta[i].sumr_hsb << 32) | ae_sta.win_sta[i].sumr_lsb;
        sum[1] = ((unsigned long long)ae_sta.win_sta[i].sumg_hsb << 32) | ae_sta.win_sta[i].sumg_lsb;
        sum[2] = ((unsigned long long)ae_sta.win_sta[i].sumb_hsb << 32) | ae_sta.win_sta[i].sumb_lsb;
        printf("ae_win[%d]  sumr = %llu, sumg = %llu, sumb = %llu\n", i, sum[0], sum[1], sum[2]);
        printf("           bp = %d, vbp = %d, wp = %d, vwp = %d\n",
            ae_sta.win_sta[i].bp, ae_sta.win_sta[i].vbp, ae_sta.win_sta[i].wp, ae_sta.win_sta[i].vwp);
    }

    return 0;
}

int ae_statistic_menu(void)
{
    int option, ae_en;

    // disable Grain-Media AE
    ae_en = 0;
    ioctl(isp_fd, AE_IOC_SET_ENABLE, &ae_en);
    printf("Disable Grain-Media AE\n");

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Get AE config\n");
        printf(" 2. Set AE config\n");
        printf(" 3. Get AE window\n");
        printf(" 4. Set AE window\n");
        printf(" 5. Get AE pre-gamma\n");
        printf(" 6. Set AE pre-gamma\n");
        printf(" 7. Get AE statistic\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            get_ae_sta_cfg();
            break;
        case 2:
            set_ae_sta_cfg();
            break;
        case 3:
            get_ae_win_cfg();
            break;
        case 4:
            set_ae_win_cfg();
            break;
        case 5:
            get_ae_pre_gamma();
            break;
        case 6:
            set_ae_pre_gamma();
            break;
        case 7:
            get_ae_sta();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int get_awb_sta_cfg(void)
{
    awb_sta_cfg_t awb_sta_cfg;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AWB_CFG, &awb_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AWB_CFG fail!");
        return ret;
    }

    printf("awb_sta_cfg:\n");
    printf("    macro_block    = %d\n", awb_sta_cfg.macro_block);
    printf("    sampling_mode  = %d\n", awb_sta_cfg.sampling_mode);
    printf("    y_mode         = %d\n", awb_sta_cfg.y_mode);
    for (i=0; i<13; i++)
        printf("    th[%2d]         = %d\n", i, awb_sta_cfg.th[i]);

    return 0;
}

static int set_awb_sta_cfg(void)
{
    awb_sta_cfg_t awb_sta_cfg;
    int macro_block, sampling_mode, y_mode;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AWB_CFG, &awb_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AWB_CFG fail!");
        return ret;
    }

    do {
        printf("macro_block (0:2x2, 1:4x4, 2:8x8, 3:16x16, 4:32x32) >> ");
        macro_block = get_choose_int();
    } while (macro_block < 0 || macro_block > 4);

    do {
        printf("sampling_mode (0:1/1, 1:1/2, 2:1/4, 3:1/8, 4:1/16) >> ");
        sampling_mode = get_choose_int();
    } while (sampling_mode < 0 || sampling_mode > 4);

    do {
        printf("y_mode (0:RGrGbB_DIV4, 1:GrGb_DIV2) >> ");
        y_mode = get_choose_int();
    } while (y_mode < 0 || y_mode > 1);

    awb_sta_cfg.macro_block = macro_block;
    awb_sta_cfg.sampling_mode = sampling_mode;
    awb_sta_cfg.y_mode = y_mode;

    return ioctl(isp_fd, ISP_IOC_SET_AWB_CFG, &awb_sta_cfg);
}

static int get_awb_win_cfg(void)
{
    awb_win_cfg_t awb_win_cfg;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AWB_WIN, &awb_win_cfg)) < 0){
        printf("ISP_IOC_GET_AWB_WIN fail!");
        return ret;
    }

    printf("window start(%d, %d), width=%d, height=%d\n",
        awb_win_cfg.awb_win.x, awb_win_cfg.awb_win.y,
        awb_win_cfg.awb_win.width, awb_win_cfg.awb_win.height);

    return 0;
}

static int set_awb_win_cfg(void)
{
    awb_win_cfg_t awb_win_cfg;
    win_rect_t act_win;
    int x, y, w, h;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_ACT_WIN, &act_win)) < 0){
        printf("ISP_IOC_GET_ACT_WIN fail!");
        return ret;
    }

    do {
        printf("x (0~%d) >> ", act_win.width - 1);
        x = get_choose_int();
    } while (x < 0 || x >= act_win.width);

    do {
        printf("y (0~%d) >> ", act_win.height - 1);
        y = get_choose_int();
    } while (y < 0 || y >= act_win.height);

    do {
        printf("width (0~%d)>> ", act_win.width - x);
        w = get_choose_int();
    } while (w < 0 || w > act_win.width - x);

    do {
        printf("height (0~%d)>> ", act_win.height - y);
        h = get_choose_int();
    } while (h < 0 || h > act_win.height - y);

    awb_win_cfg.awb_win.x = x;
    awb_win_cfg.awb_win.y = y;
    awb_win_cfg.awb_win.width = w;
    awb_win_cfg.awb_win.height = h;

    return ioctl(isp_fd, ISP_IOC_SET_AWB_WIN, &awb_win_cfg);
}

static int get_awb_hw_sta(void)
{
    awb_sta_t awb_sta;
    int tmp_int, ret;

    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AWB_STA, &awb_sta)) < 0){
        printf("ISP_IOC_GET_AWB_STA fail!");
        return ret;
    }

    printf("awb_sta:\n");
    printf("    rg_lvl        = %d\n", awb_sta.rg_lvl);
    printf("    bg_lvl        = %d\n", awb_sta.bg_lvl);
    printf("    pixel_num_lvl = %d\n", awb_sta.pixel_num_lvl);
    printf("    rg            = %d\n", awb_sta.rg);
    printf("    bg            = %d\n", awb_sta.bg);
    printf("    r             = %d\n", awb_sta.r);
    printf("    g             = %d\n", awb_sta.g);
    printf("    b             = %d\n", awb_sta.b);
    printf("    pixel_num     = %d\n", awb_sta.pixel_num);
    return 0;
}

static int set_awb_sta_mode(void)
{
    int sta_mode;
    int ret;

    ret = ioctl(isp_fd, AWB_IOC_GET_STA_MODE, &sta_mode);
    if (ret < 0)
        return ret;
    printf("AWB statistic mode = %s\n", sta_mode ? "AWB_SEG_STA" : "AWB_HW_STA");

    do {
        printf("AWB statistic mode (0:AWB_HW_STA, 1:AWB_SEG_STA) >> ");
        sta_mode = get_choose_int();
    } while ((sta_mode < 0 || sta_mode > 1));

    return ioctl(isp_fd, AWB_IOC_SET_STA_MODE, &sta_mode);
}

static int get_awb_seg_sta(void)
{
    awb_sta_cfg_t awb_sta_cfg;
    awb_seg_sta_t awb_seg_sta;
    int i, j, num_x, num_y, avg_y;
    int tmp_int, ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AWB_CFG, &awb_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AWB_CFG fail!");
        return ret;
    }
    if (!awb_sta_cfg.segment_mode) {
        printf("AWB statistic mode is not segment mode\n");
        return -1;
    }

    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);

    ret = ioctl(isp_fd, ISP_IOC_GET_AWB_SEG_STA, &awb_seg_sta);
    if (ret < 0)
        return ret;

    num_x = awb_sta_cfg.num_x;
    num_y = awb_sta_cfg.num_y;
    printf("num_x=%d, num_y=%d\n", num_x, num_y);
    printf("-------------------------------------------------\n");
    for (j=0; j<num_y; j++) {
        for (i=0; i<num_x; i++) {
            avg_y = awb_seg_sta.seg_sta[j*num_x+i].Y_sum /
                    awb_seg_sta.seg_sta[j*num_x+i].mblock_cnt;
            printf("%3d ", avg_y);
            if ((i % 16) == 15)
                printf("\n");
        }
        printf("\n");
    }
    printf("-------------------------------------------------\n");
    return ret;
}

int awb_statistic_menu(void)
{
    int option, awb_en;

    // disable Grain-Media AWB
    awb_en = 0;
    ioctl(isp_fd, AWB_IOC_SET_ENABLE, &awb_en);
    printf("Disable Grain-Media AWB\n");

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Get AWB config\n");
        printf(" 2. Set AWB config\n");
        printf(" 3. Get AWB window\n");
        printf(" 4. Set AWB window\n");
        printf(" 5. Get AWB HW statistic\n");
        printf(" 6. Set AWB statistic mode\n");
        printf(" 7. Get AWB segement statistic\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            get_awb_sta_cfg();
            break;
        case 2:
            set_awb_sta_cfg();
            break;
        case 3:
            get_awb_win_cfg();
            break;
        case 4:
            set_awb_win_cfg();
            break;
        case 5:
            get_awb_hw_sta();
            break;
        case 6:
            set_awb_sta_mode();
            break;
        case 7:
            get_awb_seg_sta();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int get_af_sta_cfg(void)
{
    af_sta_cfg_t af_sta_cfg;
    int ret, i, j;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_CFG, &af_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AF_CFG fail!");
        return ret;
    }

    printf("af_sta_cfg:\n");
    printf("    af_shift = %d\n",   af_sta_cfg.af_shift);
    printf("    wins_en  = 0x%x\n", af_sta_cfg.wins_en);
    for (i=0; i<4; i++) {
        printf("    af_hpf%d : ", i);
        for (j=0; j<6; j++)
            printf(" %d ", af_sta_cfg.af_hpf[i][j]);
        printf("\n");
    }

    return 0;
}

static int set_af_sta_cfg(void)
{
    af_sta_cfg_t af_sta_cfg;
    int af_shift, wins_en;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_CFG, &af_sta_cfg)) < 0){
        printf("ISP_IOC_GET_AF_CFG fail!");
        return ret;
    }

    do {
        printf("af_shift (0~7) >> ");
        af_shift = get_choose_int();
    } while (af_shift < 0 || af_shift > 7);

    do {
        printf("wins_en (0~0x1FF) >> 0x");
        wins_en = get_choose_hex();
    } while (wins_en < 0 || wins_en > 0x1FF);

    af_sta_cfg.af_shift = af_shift;
    af_sta_cfg.wins_en = wins_en;

    return ioctl(isp_fd, ISP_IOC_SET_AF_CFG, &af_sta_cfg);
}

static int get_af_win_cfg(void)
{
    af_win_cfg_t af_win_cfg;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_WIN, &af_win_cfg)) < 0){
        printf("ISP_IOC_GET_AF_WIN fail!");
        return ret;
    }

    for (i=0; i<9; i++) {
        printf("af_win[%d] start(%d, %d), width=%d, height=%d\n", i,
            af_win_cfg.af_win[i].x, af_win_cfg.af_win[i].y,
            af_win_cfg.af_win[i].width, af_win_cfg.af_win[i].height);
    }

    return 0;
}

static int set_af_win_cfg(void)
{
    af_win_cfg_t af_win_cfg;
    win_rect_t act_win;
    int win_idx, x, y, w, h;
    int ret;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_ACT_WIN, &act_win)) < 0){
        printf("ISP_IOC_GET_ACT_WIN fail!");
        return ret;
    }

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_WIN, &af_win_cfg)) < 0){
        printf("ISP_IOC_GET_AF_WIN fail!");
        return ret;
    }

    do {
        printf("window (0~8) >> ");
        win_idx = get_choose_int();
    } while (win_idx < 0 || win_idx > 8);

    do {
        printf("x (0~%d) >> ", act_win.width - 1);
        x = get_choose_int();
    } while (x < 0 || x >= act_win.width);

    do {
        printf("y (0~%d) >> ", act_win.height - 1);
        y = get_choose_int();
    } while (y < 0 || y >= act_win.height);

    do {
        printf("width (0~%d)>> ", act_win.width - x);
        w = get_choose_int();
    } while (w < 0 || w > act_win.width - x);

    do {
        printf("height (0~%d)>> ", act_win.height - y);
        h = get_choose_int();
    } while (h < 0 || h > act_win.height - y);

    af_win_cfg.af_win[win_idx].x = x;
    af_win_cfg.af_win[win_idx].y = y;
    af_win_cfg.af_win[win_idx].width = w;
    af_win_cfg.af_win[win_idx].height = h;

    return ioctl(isp_fd, ISP_IOC_SET_AF_WIN, &af_win_cfg);
}

static int get_af_pre_gamma(void)
{
    af_pre_gamma_t af_pre_gamma;
    int ret, i;

    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_PRE_GAMMA, &af_pre_gamma)) < 0){
        printf("ISP_IOC_GET_AF_PRE_GAMMA fail!");
        return ret;
    }

    for (i=0; i<2; i++) {
        printf("in[%d]=%5d    out[%d]=%5d    slope[%d]=%5d\n",
            i, af_pre_gamma.in[i], i, af_pre_gamma.out[i], i, af_pre_gamma.slope[i]);
    }
    printf("                               slope[%d]=%5d\n", i, af_pre_gamma.slope[2]);
    return 0;
}

static int set_af_pre_gamma(void)
{
    af_pre_gamma_t * ppre_gamma;

    af_pre_gamma_t li_gamma = {
        {64, 128}, //in
        {64, 128}, //out
        {256, 256, 256} //slope
    };
    af_pre_gamma_t hc_gamma = {
        {64, 192}, //in
        {32, 224}, //out
        {128, 384, 128} //slope
    };
    int option;

    do {
        printf("AF pre-gamma (0:Linear, 1:High contrast)>> ");
        option = get_choose_int();
    } while (option < 0 || option > 1);

    ppre_gamma = (option == 0) ? &li_gamma : &hc_gamma;
    return ioctl(isp_fd, ISP_IOC_SET_AF_PRE_GAMMA, ppre_gamma);
}

static int get_af_sta(void)
{
    af_sta_t af_sta;
    int tmp_int, ret, i;

    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if ((ret = ioctl(isp_fd, ISP_IOC_GET_AF_STA, &af_sta)) < 0){
        printf("ISP_IOC_GET_AF_STA fail!");
        return ret;
    }

    for (i=0; i<9; i++)
        printf("af_win[%d] fv = %d\n", i, af_sta.af_result_w[i]);
    return 0;
}

int af_statistic_menu(void)
{
    int option, af_en;

    // disable Grain-Media AF
    af_en = 0;
    ioctl(isp_fd, AF_IOC_SET_ENABLE, &af_en);
    printf("Disable Grain-Media AF\n");

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Get AF config\n");
        printf(" 2. Set AF config\n");
        printf(" 3. Get AF window\n");
        printf(" 4. Set AF window\n");
        printf(" 5. Get AF pre-gamma\n");
        printf(" 6. Set AF pre-gamma\n");
        printf(" 7. Get A statistic\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            get_af_sta_cfg();
            break;
        case 2:
            set_af_sta_cfg();
            break;
        case 3:
            get_af_win_cfg();
            break;
        case 4:
            set_af_win_cfg();
            break;
        case 5:
            get_af_pre_gamma();
            break;
        case 6:
            set_af_pre_gamma();
            break;
        case 7:
            get_af_sta();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int option;
    u32 id, ver;

    // open ISP device
    isp_fd = open("/dev/isp320", O_RDWR);
    if (isp_fd < 0) {
        printf("Open ISP320 fail\n");
        return -1;
    }

    // just ensure ISP is start, generally ISP has been started after insert capture driver
    ret = ioctl(isp_fd, ISP_IOC_START, NULL);
    if (ret < 0)
        return -1;

    // check chip ID
    ret = ioctl(isp_fd, ISP_IOC_GET_CHIPID, &id);
    if (ret < 0)
        return -1;
    printf("Chip ID: %x\n", id);
    if (id != 0x8139)
        return -1;

    // Version
    ret = ioctl(isp_fd, ISP_IOC_GET_DRIVER_VER, &ver);
    if (ret == 0)
        printf("ISP driver ver. 0x%08x\n", ver);
    ret = ioctl(isp_fd, ISP_IOC_GET_INPUT_INF_VER, &ver);
    if (ret == 0)
        printf("ISP input interface ver. 0x%08x\n", ver);
    ret = ioctl(isp_fd, ISP_IOC_GET_ALG_INF_VER, &ver);
    if (ret == 0)
        printf("ISP algorithm interface ver. 0x%08x\n", ver);

    while (1) {
        printf("----------------------------------------\n");
        printf(" 0. Image pipe ioctl\n");
        printf(" 1. User preference ioctl\n");
        printf(" 2. Sensor ioctl\n");
        printf(" 3. AE ioctl\n");
        printf(" 4. AWB ioctl\n");
        printf(" 5. AF ioctl\n");
        printf(" 6. Histogram ioctl\n");
        printf(" 7. AE statistic ioctl\n");
        printf(" 8. AWB statistic ioctl\n");
        printf(" 9. AF statistic ioctl\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");

        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 0:
            image_pipe_ioctl_menu();
            break;
        case 1:
            user_pref_ioctl_menu();
            break;
        case 2:
            sensor_ioctl_menu();
            break;
        case 3:
            ae_ioctl_menu();
            break;
        case 4:
            awb_ioctl_menu();
            break;
        case 5:
            af_ioctl_menu();
            break;
        case 6:
            hist_statistic_menu();
            break;
        case 7:
            ae_statistic_menu();
            break;
        case 8:
            awb_statistic_menu();
            break;
        case 9:
            af_statistic_menu();
            break;
        case 99:
            goto end;
        }
    }

end:
    close(isp_fd);
    return 0;
}
