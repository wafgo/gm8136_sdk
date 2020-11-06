#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ioctl_isp328.h"

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

static int test_size(void)
{
    int ret = 0, option;
    win_size_t win_size;
    win_rect_t win_rect;

    while(1){
        printf("----------------------------------------\n");
        printf(" 1. Get Size\n");
        printf(" 2. Set Size\n");
        printf(" 3. Get Active Window Size\n");
        printf(" 4. Set Active Window Size\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");

        do {
            printf(">> ");
            option = get_choose_int();
        } while ((option < 1) || (option > 99));

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
        case 3:
            ret = ioctl(isp_fd, ISP_IOC_GET_ACT_WIN, &win_rect);
            if (ret < 0)
                return ret;
            printf("x=%d, y=%d, width=%d, height=%d\n",
                    win_rect.x, win_rect.y, win_rect.width, win_rect.height);
            break;
        case 4:
            do {
                printf("x=");
                win_rect.x = get_choose_int();
            } while (win_rect.x < 0);
            do {
                printf("y=");
                win_rect.y = get_choose_int();
            } while (win_rect.y < 0);
            do {
                printf("width=");
                win_rect.width = get_choose_int();
            } while (win_rect.width < 0);
            do {
                printf("height=");
                win_rect.height = get_choose_int();
            } while (win_rect.height < 0);

            ret = ioctl(isp_fd, ISP_IOC_SET_ACT_WIN, &win_rect);
            if (ret < 0)
                return ret;
            break;
        case 99:
            return 0;
        }
    }
    return 0;
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

static int test_dpcd(void)
{
    int ret, i;
    dcpd_reg_t dpcd;
    dcpd_reg_t dpcd_2[2]={
        {{1024, 2048, 2944, 3712}, {1024, 2048, 16384, 65535}, {16, 16, 256, 1024}},
        {{1024, 2048, 3072, 4096}, {1024, 2048, 3072, 4096}, {16, 16, 16, 16}}
    };
    
    ret = ioctl(isp_fd, ISP_IOC_GET_DCPD, &dpcd);
    
    if (ret < 0)
        return ret;
    for (i=0; i<4; i++)
        printf("knee_x[%d] = %d\n", i, dpcd.knee_x[i]);
    for (i=0; i<4; i++)
        printf("knee_y[%d] = %d\n", i, dpcd.knee_y[i]);
    for (i=0; i<4; i++)
        printf("slope[%d] = %d\n", i, dpcd.slope[i]);
   
    do {
        printf("Select Dcompanding (0. Linear 1.WDR)\n");
        i = get_choose_int();
    } while (i < 0);

    if (i)
        i=1;
    ret = ioctl(isp_fd, ISP_IOC_SET_DCPD, &dpcd_2[i]);
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
    printf("Current BLC: %d, %d, %d, %d\n", blc.offset[0], blc.offset[1],
                                    blc.offset[2], blc.offset[3]);
    for (i=0; i<4; i++) {
        printf("BLC %s offset >> ", c_name[i]);
        blc.offset[i] = get_choose_int();
    }

    return ioctl(isp_fd, ISP_IOC_SET_BLC, &blc);
}

static int test_dpc(void)
{
    dpc_reg_t dpc_reg;
    int ret = 0, i;

    ret =ioctl(isp_fd, ISP_IOC_GET_DPC, &dpc_reg);
    if (ret < 0)
        return ret;
    for (i=0; i<3; i++)
        printf("th[%d] = %d\n", i, dpc_reg.th[i]);
    printf("eth = %d\n\n", dpc_reg.eth);

    printf("New Setting :\n\n");
    for (i=0; i<3; i++) {
        do {
        printf("th[%d] = \n", i);
        dpc_reg.th[i] = get_choose_int();
        } while (dpc_reg.th[i] < 0);
    }
    
    do {
        printf("eth = \n");
        dpc_reg.eth= get_choose_int();
    } while (dpc_reg.eth < 0);
    
    ret = ioctl(isp_fd, ISP_IOC_SET_DPC, &dpc_reg);

    return 0;
}

static int test_rdn(void)
{
    rdn_reg_t rdn_reg;
    int ret = 0, i;

    ret =ioctl(isp_fd, ISP_IOC_GET_RAW_NR, &rdn_reg);
    if (ret < 0)
        return ret;
    printf("Current RAW NR settings:\n");
    for (i=0; i<3; i++){
        printf("th[%d] = %d\n", i, rdn_reg.th[i]);
        printf("coef[%d] = %d\n", i, rdn_reg.coef[i]);
    }
    printf("kf_min = %d\n", rdn_reg.kf_min);
    printf("kf_max = %d\n", rdn_reg.kf_max);

    rdn_reg.th[0] = 16;
    rdn_reg.th[1] = 24;
    rdn_reg.th[2] = 450;
    rdn_reg.coef[0] = 8192;
    rdn_reg.coef[1] = 8192;
    rdn_reg.coef[2] = 2048;
    rdn_reg.kf_min = 16;
    rdn_reg.kf_max = 84;
    
    printf("Set RAW NR settings to:\n");
    for (i=0; i<3; i++){
        printf("th[%d] = %d\n", i, rdn_reg.th[i]);
        printf("coef[%d] = %d\n", i, rdn_reg.coef[i]);
    }
    printf("kf_min = %d\n", rdn_reg.kf_min);
    printf("kf_max = %d\n", rdn_reg.kf_max);

    ret = ioctl(isp_fd, ISP_IOC_SET_RAW_NR, &rdn_reg);
    return 0;
}

static int test_ctk(void)
{
    ctk_reg_t ctk_reg;
    int ret = 0, i;

    ret =ioctl(isp_fd, ISP_IOC_GET_CTK, &ctk_reg);
    if (ret < 0)
        return ret;
    printf("Current CTK settings:\n");
    for (i=0; i<3; i++)
        printf("th[%d] = %d\n", i, ctk_reg.th[i]);
    printf("coef1 = %d\n", ctk_reg.coef1);
    printf("kf_min = %d\n", ctk_reg.kf_min);
    printf("kf_max = %d\n", ctk_reg.kf_max);

    ctk_reg.th[0] = 16;
    ctk_reg.th[1] = 24;
    ctk_reg.th[2] = 450;
    ctk_reg.coef1 = 2048;
    ctk_reg.kf_min = 16;
    ctk_reg.kf_max = 255;
    printf("Set CTK settings to:\n");
    for (i=0; i<3; i++)
        printf("th[%d] = %d\n", i, ctk_reg.th[i]);
    printf("coef1 = %d\n", ctk_reg.coef1);
    printf("kf_min = %d\n", ctk_reg.kf_min);
    printf("kf_max = %d\n", ctk_reg.kf_max);

    ret = ioctl(isp_fd, ISP_IOC_SET_CTK, &ctk_reg);
    return 0;
}

static void dump_lsc_reg(lsc_reg_t *plsc)
{
    int i, j;
    const char *c_name[] = {"R", "Gr", "Gb", "B"};

    printf("segrds = %d\n", plsc->segrds);
    for (i=0; i<4; i++)
        printf("%s center (%d, %d)\n", c_name[i], plsc->center[i].x, plsc->center[i].y);

    for (j=0; j<4; j++) {
        printf("lsc_mxtn_%s = ", c_name[j]);
        for (i=0; i<8; i++)
            printf("%d ", plsc->maxtan[j][i]);
        printf("\n");
    }

    for (j=0; j<4; j++) {
        printf("lsc_rdsparam_%s = ", c_name[j]);
        for (i=0; i<16; i++)
            printf("%d ", plsc->radparam[j][i]);
        printf("...\n");
    }
}

static int test_lsc(void)
{
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
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_LSC, &lsc);
    if (ret < 0)
        return ret;

    printf("Current LSC setting:\n");
    dump_lsc_reg(&lsc);
    
    printf("New LSC setting:\n");
    dump_lsc_reg(&lsc_test);
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

    ret = ioctl(isp_fd, ISP_IOC_GET_GAMMA_LUT, &gamma);
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
        ret = ioctl(isp_fd, ISP_IOC_SET_GAMMA_LUT, &bt709);
    } else {
        ret = ioctl(isp_fd, ISP_IOC_SET_GAMMA_LUT, &r22);
    }

    return ret;
}

static int test_ce(void)
{
    int ret, option, i, j;
    ce_cfg_t ce_cfg;
    int ce_str;
    ce_hist_t ce_hist;
    ce_curve_t ce_curve;

    while(1){
        printf("------------------------------------------\n");
        printf(" 1. Get config\n");
        printf(" 2. Set config\n");
        printf(" 3. Get strength\n");
        printf(" 4. Set strength\n");
        printf(" 5. Get histogram\n");
        printf(" 6. Get tone mapping curve\n");
        printf(" 7. Set tone mapping curve\n");
        printf("99. Quit\n");
        printf("------------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 1);

        switch (option) {
        case 1:
            ret = ioctl(isp_fd, ISP_IOC_GET_CE_CFG, &ce_cfg);
            if (ret < 0)
                return ret;
            printf("Current config:\n");
            printf("bghno[0] = %d\n", ce_cfg.bghno[0]);
            printf("bgvno[0] = %d\n", ce_cfg.bgvno[0]);
            printf("bghno[1] = %d\n", ce_cfg.bghno[1]);
            printf("bgvno[1] = %d\n", ce_cfg.bgvno[1]);
            printf("blend = %d\n", ce_cfg.blend);
            printf("th = %d\n", ce_cfg.th);
            printf("strength = %d\n", ce_cfg.strength);
            break;
        case 2:
            ce_cfg.bghno[0] = 16;
            ce_cfg.bgvno[0] = 9;
            ce_cfg.bghno[1] = 8;
            ce_cfg.bgvno[1] = 4;
            ce_cfg.blend = 128;
            ce_cfg.th = 3;
            ce_cfg.strength = 30;

            printf("Set new config:\n");
            printf("bghno[0] = %d\n", ce_cfg.bghno[0]);
            printf("bgvno[0] = %d\n", ce_cfg.bgvno[0]);
            printf("bghno[1] = %d\n", ce_cfg.bghno[1]);
            printf("bgvno[1] = %d\n", ce_cfg.bgvno[1]);
            printf("blend = %d\n", ce_cfg.blend);
            printf("th = %d\n", ce_cfg.th);
            printf("strength = %d\n", ce_cfg.strength);
            ret = ioctl(isp_fd, ISP_IOC_SET_CE_CFG, &ce_cfg);
            if (ret < 0)
                return ret;
            break;
        case 3:
            ret = ioctl(isp_fd, ISP_IOC_GET_CE_STRENGTH, &ce_str);
            if (ret < 0)
                return ret;
            printf("current strength = %d\n", ce_str);
            break;
        case 4:
            ret = ioctl(isp_fd, ISP_IOC_GET_CE_STRENGTH, &ce_str);
            if (ret < 0)
                return ret;
            printf("current strength = %d\n", ce_str);
            do {
                printf(">> ");
                ce_str = get_choose_int();
            } while ((ce_str < 1) && (ce_str > 255));
            ret = ioctl(isp_fd, ISP_IOC_SET_CE_STRENGTH, &ce_str);
            break;
        case 5:
            ret = ioctl(isp_fd, ISP_IOC_GET_CE_HISTOGRAM, &ce_hist);
            if (ret < 0)
                return ret;
            printf("current histogram =\n");
            for (i=0;i<16;i++){
                for (j=0;j<16;j++)
                    printf(" %d", ce_hist.bin[i*16+j]);
                printf("\n");
            }
            break;
        case 6:
            ret = ioctl(isp_fd, ISP_IOC_GET_CE_TONE_CURVE, &ce_curve);
            if (ret < 0)
                return ret;
            printf("current tone curve =\n");
            for (i=0;i<16;i++){
                for (j=0;j<16;j++)
                    printf(" %d", ce_curve.curve[i*16+j]);
                printf("\n");
            }
            break;
        case 7:
            ret = ioctl(isp_fd, ISP_IOC_SET_CE_TONE_CURVE, &ce_curve);
            if (ret < 0)
                return ret;
            for (i=0; i<256; i++)
                ce_curve.curve[i] = i/2;

            printf("set tone curve =\n");
            for (i=0;i<16;i++){
                for (j=0;j<16;j++)
                    printf(" %d", ce_curve.curve[i*16+j]);
                printf("\n");
            }
            ret = ioctl(isp_fd, ISP_IOC_SET_CE_TONE_CURVE, &ce_curve);
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int test_ci_cfg(void)
{
    ci_reg_t ci;
    int ret, option;

    while(1){
        printf("------------------------------------------\n");
        printf(" 1. Get Current Bayer interpolation config\n");
        printf(" 2. Set Current Bayer interpolation config\n");
        printf("99. Quit\n");
        printf("------------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 1);

        switch (option) {
        case 1:
            ret = ioctl(isp_fd, ISP_IOC_GET_CI_CFG, &ci);
            if (ret < 0)
                return ret;
            printf("edge_dth=%d\n", ci.edge_dth);
            printf("freq_th=%d\n", ci.freq_th);
            printf("freq_blend=%d\n", ci.freq_blend);
            break;
        case 2:
            ci.edge_dth = 20;
            ci.freq_th = 320;
            ci.freq_blend = 16;
            printf("set new config:\n");        
            printf("edge_dth=%d\n", ci.edge_dth);
            printf("freq_th=%d\n", ci.freq_th);
            printf("freq_blend=%d\n", ci.freq_blend);
            ret = ioctl(isp_fd, ISP_IOC_SET_CI_CFG, &ci);
            if (ret < 0)
                return ret;
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int test_ci_nr(void)
{
    int option;
    int ret = 0;
    cdn_reg_t cdn_reg;

    while(1){
        printf("----------------------------------------\n");
        printf(" 1. Get RGB NR settings\n");
        printf(" 2. Set RGB NR settings\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 1);

        switch (option) {
        case 1:
            ret = ioctl(isp_fd, ISP_IOC_GET_CI_NR, &cdn_reg);
            if (ret < 0)
                return ret;
            printf("Current settings\n");
            printf("th[]=%5d, %5d, %5d\n", cdn_reg.th[0], cdn_reg.th[1], cdn_reg.th[2]);
            printf("dth=%5d\n", cdn_reg.dth);
            printf("kf_min=%5d\n", cdn_reg.kf_min);
            printf("kf_max=%5d\n", cdn_reg.kf_max);
            
            break;
        case 2:
            cdn_reg.th[0] = 3;
            cdn_reg.th[1] = 6;
            cdn_reg.th[2] = 1;
            cdn_reg.dth = 20;
            cdn_reg.kf_min = 0;
            cdn_reg.kf_max = 128;
            printf("Set new settings\n");
            printf("th[]=%5d, %5d, %5d\n", cdn_reg.th[0], cdn_reg.th[1], cdn_reg.th[2]);
            printf("dth=%5d\n", cdn_reg.dth);
            printf("kf_min=%5d\n", cdn_reg.kf_min);
            printf("kf_max=%5d\n", cdn_reg.kf_max);

            ret = ioctl(isp_fd, ISP_IOC_SET_CI_NR, &cdn_reg);
            if (ret < 0)
                return ret;
            break;
        case 99:
            return 0;
        }
    }
    return 0;
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

    while(1){
        printf("----------------------------------------\n");
        printf(" 1. Get Current CC Matrix\n");
        printf(" 2. Set Test CC Matrx\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 1);

        switch (option) {
        case 1:
            ret = ioctl(isp_fd, ISP_IOC_GET_CC_MATRIX, &cc);
            if (ret < 0)
                return ret;
            printf("Current CC Matrix:\n");
            for (i=0; i<3; i++)
                printf("%5d, %5d, %5d\n", cc.coe[i*3], cc.coe[i*3+1], cc.coe[i*3+2]);
            break;
        case 2:
            printf("Set new CC Matrix\n");
            for (i=0; i<3; i++)
                printf("%5d, %5d, %5d\n", cc_test.coe[i*3], cc_test.coe[i*3+1], cc_test.coe[i*3+2]);
            ret = ioctl(isp_fd, ISP_IOC_SET_CC_MATRIX, &cc_test);
            if (ret < 0)
                return ret;
            break;
        case 99:
            return 0;
        }
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

    while(1){
        printf("----------------------------------------\n");
        printf(" 1. Get Current CV Matrix\n");
        printf(" 2. Set Test CV Matrx\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 1);

        switch (option) {
        case 1:
            ret = ioctl(isp_fd, ISP_IOC_GET_CV_MATRIX, &cv);
            if (ret < 0)
                return ret;
            printf("Current CV Matrix:\n");
            for (i=0; i<3; i++)
                printf("%5d, %5d, %5d\n", (int)cv.coe[i*3], (int)cv.coe[i*3+1], (int)cv.coe[i*3+2]);
            break;
        case 2:
            printf("Set new CV Matrix\n");
            for (i=0; i<3; i++)
                printf("%5d, %5d, %5d\n", cv_test.coe[i*3], cv_test.coe[i*3+1], cv_test.coe[i*3+2]);
            ret = ioctl(isp_fd, ISP_IOC_SET_CV_MATRIX, &cv_test);
            if (ret < 0) {
                printf("ISP_IOC_SET_CV_MATRIX failed\n");
                return ret;
            }
            break;
        case 99:
            return 0;
        }
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

static int set_mrnr_en(void)
{
    int mrnr_en;

    do {
        printf("Post Noise Reduction (0:Disable, 1:Enable) >> ");
        mrnr_en = get_choose_int();
    } while (mrnr_en < 0);

    return ioctl(isp_fd, ISP_IOC_SET_MRNR_EN, &mrnr_en);
}

static void dump_mrnr_param(mrnr_param_t *pmrnr_param)
{
    int i, j;

    printf("Y_L_edg_th=\n");
    for (i=0;i<4;i++){
        for (j=0;j<8;j++)
            printf("%d, ", pmrnr_param->Y_L_edg_th[i][j]);
        printf("\n");
    }
    printf("Cb_L_edg_th=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->Cb_L_edg_th[i]);
    printf("\n");
    printf("Cr_L_edg_th=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->Cr_L_edg_th[i]);
    printf("\n");

    printf("Y_L_smo_th=\n");
    for (i=0;i<4;i++){
        for (j=0;j<8;j++)
            printf("%d, ", pmrnr_param->Y_L_smo_th[i][j]);
        printf("\n");
    }
    printf("Cb_L_smo_th=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->Cb_L_smo_th[i]);
    printf("\n");
    printf("Cr_L_smo_th=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->Cr_L_smo_th[i]);
    printf("\n");

    printf("Y_L_nr_str=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->Y_L_nr_str[i]);
    printf("\n");

    printf("C_L_nr_str=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_param->C_L_nr_str[i]);
    printf("\n");
}

static int get_mrnr_param(void)
{
    int ret;
    mrnr_param_t mrnr_param;

    ret = ioctl(isp_fd, ISP_IOC_GET_MRNR_PARAM, &mrnr_param);
    
    printf("Current mrnr parameters:\n");

    dump_mrnr_param(&mrnr_param);
    
    return ret;
}

static int set_mrnr_param(void)
{
    mrnr_param_t mrnr_param={
        {{0, 0, 6, 8, 9,10, 6, 4},
         {0, 0, 3, 4, 5, 5, 3, 2},
         {0, 0, 2, 2, 3, 3, 2, 1},
         {0, 0, 1, 1, 1, 1, 1, 0}},
         {0, 0, 0, 0},
         {0, 0, 0, 0},
        {{0, 0, 3, 4, 4, 4, 4, 3},
         {0, 0, 2, 3, 3, 3, 3, 2},
         {0, 0, 1, 2, 2, 2, 2, 1},
         {0, 0, 1, 1, 1, 1, 1, 1}},
         {0, 0, 0, 0},
         {0, 0, 0, 0},
         {8, 8, 8, 8},
         {5, 5, 5, 5}
    };

    printf("Set mrnr parameters:\n");

    dump_mrnr_param(&mrnr_param);

    return ioctl(isp_fd, ISP_IOC_SET_MRNR_PARAM, &mrnr_param);
}

static void dump_mrnr_operator(mrnr_op_t *pmrnr_op)
{
    int i, j;

    printf("G1_Y_Freq=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->G1_Y_Freq[i]);
    printf("\n");

    printf("G1_Y_Tone=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->G1_Y_Tone[i]);
    printf("\n");

    printf("G1_C=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->G1_C[i]);
    printf("\n");

    printf("G2_Y=\n");
    printf("%d, ", pmrnr_op->G2_Y);
    printf("\n");

    printf("G2_C=\n");
    printf("%d, ", pmrnr_op->G2_C);
    printf("\n");

    printf("Y_L_Nr_Str=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->Y_L_Nr_Str[i]);
    printf("\n");

    printf("C_L_Nr_Str=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->C_L_Nr_Str[i]);
    printf("\n");

    printf("Y_noise_lvl=\n");
    for (i=0;i<4;i++){
        for (j=0;j<8;j++)
            printf("%d, ", pmrnr_op->Y_noise_lvl[i*8+j]);
        printf("\n");
    }

    printf("Cb_noise_lvl=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->Cb_noise_lvl[i]);
    printf("\n");

    printf("Cr_noise_lvl=\n");
    for (i=0;i<4;i++)
        printf("%d, ", pmrnr_op->Cr_noise_lvl[i]);
    printf("\n");
}

static int get_mrnr_operator(void)
{
    int ret;
    mrnr_op_t mrnr_op;

    ret = ioctl(isp_fd, ISP_IOC_GET_MRNR_OPERATOR, &mrnr_op);

    printf("Current mrnr operator:\n");

    dump_mrnr_operator(&mrnr_op);
    
    return ret;
}

static int set_mrnr_operator(void)
{
    mrnr_op_t mrnr_op={
        {30,22,19,16},
        {13,19,19,13},
        {26,26,26,26},
        144,
        144,
        {8,8,8,8},
        {5,5,5,5},
        {46,61,63,48,30,25,20,18,
         34,45,47,36,22,18,15,13,
         23,30,31,24,15,12,10, 9,
         11,15,15,12, 7, 6, 5, 4},
        {1,1,1,1},
        {1,1,1,1}
        };

    printf("Set mrnr operator:\n");

    dump_mrnr_operator(&mrnr_op);
        
    return ioctl(isp_fd, ISP_IOC_SET_MRNR_OPERATOR, &mrnr_op);
}

static int test_mrnr(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable / Disable\n");
        printf(" 2. Get Param\n");
        printf(" 3. Set Param\n");
        printf(" 4. Get Operator\n");
        printf(" 5. Set Operator\n");
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
            get_mrnr_param();
            break;
        case 3:
            set_mrnr_param();
            break;
        case 4:
            get_mrnr_operator();
            break;
        case 5:
            set_mrnr_operator();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
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

static void dump_tmnr_param(tmnr_param_t *ptmnr_param)
{
    printf("Y_var = %d\n", ptmnr_param->Y_var);
    printf("Cb_var = %d\n", ptmnr_param->Cb_var);
    printf("Cr_var = %d\n", ptmnr_param->Cr_var);
    printf("K = %d\n", ptmnr_param->K);
    printf("suppress_strength = %d\n", ptmnr_param->suppress_strength);
    printf("NF = %d\n", ptmnr_param->NF);
    printf("var_offset = %d\n", ptmnr_param->var_offset);
    printf("motion_var = %d\n", ptmnr_param->motion_var);
    printf("trade_thres = %d\n", ptmnr_param->trade_thres);
    printf("auto_recover = %d\n", ptmnr_param->auto_recover);
    printf("auto_recover = %d\n", ptmnr_param->rapid_recover);
    printf("Motion_top_lft_th = %d\n", ptmnr_param->Motion_top_lft_th);
    printf("Motion_top_nrm_th = %d\n", ptmnr_param->Motion_top_nrm_th);
    printf("Motion_top_rgt_th = %d\n", ptmnr_param->Motion_top_rgt_th);
    printf("Motion_nrm_lft_th = %d\n", ptmnr_param->Motion_nrm_lft_th);
    printf("Motion_nrm_nrm_th = %d\n", ptmnr_param->Motion_nrm_nrm_th);
    printf("Motion_nrm_rgt_th = %d\n", ptmnr_param->Motion_nrm_rgt_th);
    printf("Motion_bot_lft_th = %d\n", ptmnr_param->Motion_bot_lft_th);
    printf("Motion_bot_nrm_th = %d\n", ptmnr_param->Motion_bot_nrm_th);
    printf("Motion_bot_rgt_th = %d\n", ptmnr_param->Motion_bot_rgt_th);
}

static int get_tmnr_param(void)
{
    int ret;
    tmnr_param_t tmnr_param;

    ret = ioctl(isp_fd, ISP_IOC_GET_TMNR_PARAM, &tmnr_param);
    dump_tmnr_param(&tmnr_param);

    return ret;
}

static int set_tmnr_param(void)
{
    tmnr_param_t tmnr_param={6,6,6,2,18,5,6,5,64,0,0,
                            50,64,14,50,64,14,25,32,7};

    dump_tmnr_param(&tmnr_param);
    
    return ioctl(isp_fd, ISP_IOC_SET_TMNR_PARAM, &tmnr_param);
}

static int test_tmnr(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable / Disable\n");
        printf(" 2. Get Param\n");
        printf(" 3. Set Param\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            set_tmnr_en();
            break;
        case 2:
            get_tmnr_param();
            break;
        case 3:
            set_tmnr_param();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}
static int sp_en(int enable)
{
    int ret;
    if (enable)
        printf("Enable Sharpness\n");
    else
        printf("Disble Sharpness\n");
    ret = ioctl(isp_fd, ISP_IOC_GET_SP_EN, enable);

    return 0;
}

static void dump_sp_param(sp_param_t *psp_param)
{
    int i, j;

    printf("hpf_enable=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_enable[i]);
    printf("\n");

    printf("hpf_use_lpf=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_use_lpf[i]);
    printf("\n");

    printf("nl_gain_enable=\n");
    printf("%d, ", psp_param->nl_gain_enable);
    printf("\n");

    printf("edg_wt_enable=\n");
    printf("%d, ", psp_param->edg_wt_enable);
    printf("\n");

    printf("gau_5x5_coeff=\n");
    for (i=0;i<5;i++){
        for (j=0;j<5;j++)
            printf("%d, ", psp_param->gau_5x5_coeff[i*5+j]);
        printf("\n");
    }

    printf("gau_shift=\n");
    printf("%d, ", psp_param->gau_shift);
    printf("\n");

    printf("hpf0_5x5_coeff=\n");
    for (i=0;i<5;i++){
        for (j=0;j<5;j++)
            printf("%d, ", psp_param->hpf0_5x5_coeff[i*5+j]);
        printf("\n");
    }

    printf("hpf1_3x3_coeff=\n");
    for (i=0;i<3;i++){
        for (j=0;j<3;j++)
            printf("%d, ", psp_param->hpf1_3x3_coeff[i*3+j]);
        printf("\n");
    }

    printf("hpf2_1x5_coeff=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->hpf2_1x5_coeff[i]);
    printf("\n");

    printf("hpf3_1x5_coeff=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->hpf3_1x5_coeff[i]);
    printf("\n");

    printf("hpf4_5x1_coeff=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->hpf4_5x1_coeff[i]);
    printf("\n");

    printf("hpf5_5x1_coeff=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->hpf5_5x1_coeff[i]);
    printf("\n");

    printf("hpf_gain=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_gain[i]);
    printf("\n");

    printf("hpf_shift=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_shift[i]);
    printf("\n");

    printf("hpf_coring_th=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_coring_th[i]);
    printf("\n");

    printf("hpf_bright_clip=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_bright_clip[i]);
    printf("\n");

    printf("hpf_dark_clip=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_dark_clip[i]);
    printf("\n");

    printf("hpf_peak_gain=\n");
    for (i=0;i<6;i++)
        printf("%d, ", psp_param->hpf_peak_gain[i]);
    printf("\n");

    printf("rec_bright_clip=\n");
    printf("%d, ", psp_param->rec_bright_clip);
    printf("\n");

    printf("rec_dark_clip=\n");
    printf("%d, ", psp_param->rec_dark_clip);
    printf("\n");

    printf("rec_peak_gain=\n");
    printf("%d, ", psp_param->rec_peak_gain);
    printf("\n");

    printf("nl_gain_curve.index=\n");
    for (i=0;i<4;i++)
        printf("%d, ", psp_param->nl_gain_curve.index[i]);
    printf("\n");
    printf("nl_gain_curve.value=\n");
    for (i=0;i<4;i++)
        printf("%d, ", psp_param->nl_gain_curve.value[i]);
    printf("\n");
    printf("nl_gain_curve.slope=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->nl_gain_curve.slope[i]);
    printf("\n");

    printf("edg_wt_curve.index=\n");
    for (i=0;i<4;i++)
        printf("%d, ", psp_param->edg_wt_curve.index[i]);
    printf("\n");
    printf("edg_wt_curve.value=\n");
    for (i=0;i<4;i++)
        printf("%d, ", psp_param->edg_wt_curve.value[i]);
    printf("\n");
    printf("edg_wt_curve.slope=\n");
    for (i=0;i<5;i++)
        printf("%d, ", psp_param->edg_wt_curve.slope[i]);
    printf("\n");
}

static int sp_parameter(int set)
{
    sp_param_t sp_param_default={
        {257, 0, 0, 1, 0, 0}, 
        {65536, 65538, 0, 262146, 2, 65536}, 
        65538, 
        0, 
        //gau_5x5_coeff
        { 0,  0,  0,   0,   8, 
         -2, -6, -10, -6,  -2, 
         -6,  0,  12,  0,  -6, 
        -10, 12,  48, 12, -10, 
         -6,  0,  12,  0,  -6}, 
        254, 
        //hpf0_5x5_coeff
        {-6, -10, -6, -2, -1, 
         -2,  -1, -2, 12, -2, 
         -1,  -2, -1,  0,  0, 
          1,   0,  0,  0,  0, 
          1,   0,  0,  0,  0}, 
        //hpf1_3x3_coeff
        {1, 0,  0, 
         0, 0,  1, 
         0, 0, 68}, 
        {41, 0, 0, 0, 0}, 
        {1031, 0, 0, 514, 0}, 
        {0, 24640, -32640, -32640, -32640}, 
        {-32640, -32640, 4112, 4112, 4112}, 
        {32896, 16, 7171, 28997, 32860, 6765}, 
        {85, 15, 184, 0, 197, 255}, 
        {15, 255, 0, 0, 5, 32}, 
        {64, 200, 16, 128, 128, 128}, 
        {153, 1, 18, 2, 0, 0}, 
        {0, 0, 175, 255, 8, 0}, 
        0, 
        0, 
        8, 
        //nl_gain_curve
        {{0, 0, 8, 0}, 
        {0, 0, 8, 0}, 
        {0, 8, 0, -10772, 1}},
        //edg_wt_curve
        {{40, 44, 190, 126}, 
        {208, 45, 190, 126}, 
        {11332, 32446, -10772, 1, 0}}
};
    sp_param_t sp_param;
    int ret;

    if (set){
        printf("Set Sharpness Parameters:\n");
        dump_sp_param(&sp_param_default);
        ret = ioctl(isp_fd, ISP_IOC_SET_SP_PARAM, &sp_param);
    }
    else{
        ret = ioctl(isp_fd, ISP_IOC_GET_SP_PARAM, &sp_param);
        printf("Current Sharpness Parameters:\n");
        dump_sp_param(&sp_param);
    }

    return ret;
}

static int test_sp(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Sharpness enable\n");
        printf(" 2. Sharpness disable\n");
        printf(" 3. Get Sharpness Parameters\n");
        printf(" 4. Set Sharpness Parameters\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            sp_en(1);
            break;
        case 2:
            sp_en(0);
            break;
        case 3:
            sp_parameter(0);
            break;
        case 4:
            sp_parameter(1);
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

    ret = ioctl(isp_fd, ISP_IOC_WAIT_STA_READY, &tmp_int);
    if ((ret = ioctl(isp_fd, ISP_IOC_GET_HISTOGRAM, &hist_sta)) < 0){
        printf("ISP_IOC_GET_HISTOGRAM fail!");
        return ret;
    }

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
        printf(" 7. Get AF statistic\n");
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

int image_pipe_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Image size\n");
        printf(" 2. Function enable/disable\n");
        printf(" 3. Dcompanding\n");
        printf(" 4. Black level correction\n");
        printf(" 5. Defect pixel correction\n");
        printf(" 6. Raw domain noise reduction\n");
        printf(" 7. De-crosstalk\n");
        printf(" 8. Lens shading correction\n");
        printf(" 9. ISP digital gain\n");
        printf("10. RGB gain\n");
        printf("11. Dynamic range correction\n");
        printf("12. Gamma\n");
        printf("13. Contrast enhancement\n");
        printf("14. Demosaic: Bayer interpolation\n");
        printf("15. RGB domain noise reduction\n");
        printf("16. RGB to RGB matrix\n");
        printf("17. RGB to YUV matrix\n");
        printf("18. Image adjustment\n");
        printf("19. Chroma suppression\n");
        printf("20. MRNR\n");
        printf("21. TMNR\n");
        printf("22. Sharpness\n");
        printf("23. Hisotgram\n");
        printf("24. AE statistic\n");
        printf("25. AWB statistic\n");
        printf("26. AF statistic\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            test_size();
            break;
        case 2:
            test_func_en();
            break;
        case 3:
            test_dpcd();
            break;
        case 4:
            test_blc();
            break;
        case 5:
            test_dpc();
            break;
        case 6:
            test_rdn();
            break;
        case 7:
            test_ctk();
            break;
        case 8:
            test_lsc();
            break;
        case 9:
            test_isp_gain();
            break;
        case 10:
            test_rgb_gain();
            break;
        case 11:
            test_drc();
            break;
        case 12:
            test_gamma();
            break;
        case 13:
            test_ce();
            break;
        case 14:
            test_ci_cfg();
            break;
        case 15:
            test_ci_nr();
            break;
        case 16:
            test_cc();
            break;
        case 17:
            test_cv();
            break;
        case 18:
            test_ia();
            break;
        case 19:
            test_cs();
            break;
        case 20:
            test_mrnr();
            break;
        case 21:
            test_tmnr();
            break;
        case 22:
            test_sp();
            break;
        case 23:
            hist_statistic_menu();
            break;
        case 24:
            ae_statistic_menu();
            break;
        case 25:
            awb_statistic_menu();
            break;
        case 26:
            af_statistic_menu();
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
    printf("Current shapness strength = %d\n", shapness);

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

static int get_adj_cc_matrix(void)
{
    adj_matrix_t adj_cc;
    int i, j;
    int ret = 0;

    ret = ioctl(isp_fd, ISP_IOC_GET_ADJUST_CC_MATRIX, &adj_cc);
    if (ret < 0)
        return ret;
    printf("Current cc matrix :\n");
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cc.matrix[j][i]);
        printf("\n");
    }

    return ret;

}

static int set_adj_cc_matrix(void)
{
    int i, j;
    adj_matrix_t adj_cc = {
        {{256,0,0,0,250,0,0,0,240},
         {250,0,0,0,240,0,0,0,230},
         {240,0,0,0,230,0,0,0,220}}
    };
    printf("set cc matrix :\n");
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cc.matrix[j][i]);
        printf("\n");
    }
    
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
    printf("Current cv matrix :\n");
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cv.matrix[j][i]);
        printf("\n");
    }

    return ret;
}

static int set_adj_cv_matrix(void)
{
    int i, j;
    adj_matrix_t adj_cv = {
        {{76,150,30,-44,-84,128,128,-108,-20},
         {76,150,30,-44,-84,128,128,-108,-20},
         {76,150,30,-44,-84,128,128,-108,-20}}
    };

    printf("set cv matrix :\n");
    for (j=0; j<3; j++) {
        for (i=0; i<9; i++)
            printf("%5d, ", adj_cv.matrix[j][i]);
        printf("\n");
    }
    
    return ioctl(isp_fd, ISP_IOC_SET_ADJUST_CV_MATRIX, &adj_cv);
}

static int auto_adjustment(void)
{
    int option, enable;
    adj_param_t adj_param;
    adj_param_t adj_param_default={
        {96,128,256,512,1024,1984,1984,1984,1984},
        {{-32,-32,-32},{-32,-32,-32},{-32,-32,-32},
        {-32,-32,-32},{-32,-32,-32},{-32,-32,-32},
        {-32,-32,-32},{-32,-32,-32},{-32,-32,-32}},
        {0,0,0,100,175,195,195,195,195},
        {6,16,28,42,43,76,76,76,76},
        {6,16,28,42,43,76,76,76,76},
        {6,16,40,99,88,98,98,98,98},
        {4,4,4,4,4,4,4,4,4},
        {20,20,10,10,10,10,10,10,10},
        {128,128,128,128,78,64,64,64,64},
        {120,120,110,110,100,80,80,80,80},
        {0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0},
        {16,16,20,32,64,64,64,64,64}
    };
    
    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Enable All adjustment\n");
        printf(" 2. Disable All adjustment\n");
        printf(" 3. Get BLC adjustment\n");
        printf(" 4. Set BLC adjustment\n");
        printf(" 5. Get NR adjustment\n");
        printf(" 6. Set NR adjustment\n");
        printf(" 7. Get Gamma adjustment\n");
        printf(" 8. Set Gamma adjustment\n");
        printf(" 9. Get Saturation adjustment\n");
        printf("10. Set Saturation adjustment\n");
        printf("11. Get Contrast adjustment\n");
        printf("12. Set Contrast adjustment\n");
        printf("13. Get Auto Adjustment parameters\n");
        printf("14. Set Auto Adjustment parameters\n");
        printf("15. Get RGB to YUV adjustment\n");
        printf("16. Set RGB to YUV adjustment\n");
        printf("17. Get Color Correction adjustment\n");
        printf("18. Set Color Correction adjustment\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");

        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            enable = 1;
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_ALL_EN, &enable);
            break;
        case 2:
            enable = 0;
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_ALL_EN, &enable);
            break;
        case 3:
            ioctl(isp_fd, ISP_IOC_GET_ADJUST_BLC_EN, &enable);
            if (enable)
                printf("BLC adjustment is enabled\n");
            else
                printf("BLC adjustment is disabled\n");
            break;
        case 4:
            do {
                printf("BLC adjustment (0:Disable, 1:Enable) >> ");
                enable = get_choose_int();
            } while (enable < 0 || enable > 1);
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_BLC_EN, &enable);
            break;
        case 5:
            ioctl(isp_fd, ISP_IOC_GET_ADJUST_NR_EN, &enable);
            if (enable)
                printf("NR adjustment is enabled\n");
            else
                printf("NR adjustment is disabled\n");
            break;
        case 6:
            do {
                printf("NR adjustment (0:Disable, 1:Enable) >> ");
                enable = get_choose_int();
            } while (enable < 0 || enable > 1);
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_NR_EN, &enable);
            break;
        case 7:
            ioctl(isp_fd, ISP_IOC_GET_ADJUST_GAMMA_EN, &enable);
            if (enable)
                printf("Gamma adjustment is enabled\n");
            else
                printf("Gamma adjustment is disabled\n");
            break;
        case 8:
            do {
                printf("Gamma adjustment (0:Disable, 1:Enable) >> ");
                enable = get_choose_int();
            } while (enable < 0 || enable > 1);
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_GAMMA_EN, &enable);
            break;
        case 9:
            ioctl(isp_fd, ISP_IOC_GET_ADJUST_SAT_EN, &enable);
            if (enable)
                printf("Saturation adjustment is enabled\n");
            else
                printf("Saturation adjustment is disabled\n");
            break;
        case 10:
            do {
                printf("Saturation adjustment (0:Disable, 1:Enable) >> ");
                enable = get_choose_int();
            } while (enable < 0 || enable > 1);
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_SAT_EN, &enable);
            break;
        case 11:
            ioctl(isp_fd, ISP_IOC_GET_ADJUST_CE_EN, &enable);
            if (enable)
                printf("Contrast adjustment is enabled\n");
            else
                printf("Contrast adjustment is disabled\n");
            break;
        case 12:
            do {
                printf("Contrast adjustment (0:Disable, 1:Enable) >> ");
                enable = get_choose_int();
            } while (enable < 0 || enable > 1);
            ioctl(isp_fd, ISP_IOC_SET_ADJUST_CE_EN, &enable);
            break;
        case 13:
            ioctl(isp_fd, ISP_IOC_GET_AUTO_ADJ_PARAM, &adj_param);
            printf("Current Adjustment parameters are :\n");
            printf("gain_th= %d, %d,...\n", adj_param.gain_th[0], adj_param.gain_th[1]);           
            printf("blc    = %d, %d,...\n", adj_param.blc[0][0], adj_param.blc[0][1]);           
            printf("dpc_lvl= %d, %d,...\n", adj_param.dpc_lvl[0], adj_param.dpc_lvl[1]);           
            printf("...\n");
            break;
        case 14:
            printf("Set Adjustment parameters to :\n");
            printf("gain_th= %d, %d,...\n", adj_param_default.gain_th[0], adj_param_default.gain_th[1]);           
            printf("blc    = %d, %d,...\n", adj_param_default.blc[0][0], adj_param_default.blc[0][1]);           
            printf("dpc_lvl= %d, %d,...\n", adj_param_default.dpc_lvl[0], adj_param_default.dpc_lvl[1]);           
            printf("...\n");
            ioctl(isp_fd, ISP_IOC_SET_AUTO_ADJ_PARAM, &adj_param_default);
            break;
        case 15:
            get_adj_cv_matrix();
            break;
        case 16:
            set_adj_cv_matrix();
    break;
        case 17:
            get_adj_cc_matrix();
            break;
        case 18:
            set_adj_cc_matrix();
            break;
        case 99:
            return 0;
        }
    }

    return 0;
}

static int fast_tuning(void)
{
    int option, level;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1. Set DPC Level\n");
        printf(" 2. Set RDN Level\n");
        printf(" 3. Set CTK Level\n");
        printf(" 4. Set CDN Level\n");
        printf(" 5. Set MRNR Level\n");
        printf(" 6. Set TMNR Level\n");
        printf(" 7. Set SP Level\n");
        printf(" 8. Set SP NR Level\n");
        printf(" 9. Set SP CLIP Level\n");
        printf("10. Set SAT Level\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");

        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);
        if (option >= 1 && option <= 10){
            do {
                printf("Select level (0~255) >> ");
                level = get_choose_int();
            } while ((level < 0) || (level > 255));
        }
        switch (option) {
        case 1:
            ioctl(isp_fd, ISP_IOC_SET_DPC_LEVEL, &level);
            break;
        case 2:
            ioctl(isp_fd, ISP_IOC_SET_RDN_LEVEL, &level);
            break;
        case 3:
            ioctl(isp_fd, ISP_IOC_SET_CTK_LEVEL, &level);
            break;
        case 4:
            ioctl(isp_fd, ISP_IOC_SET_CDN_LEVEL, &level);
            break;
        case 5:
            ioctl(isp_fd, ISP_IOC_SET_MRNR_LEVEL, &level);
            break;
        case 6:
            ioctl(isp_fd, ISP_IOC_SET_TMNR_LEVEL, &level);
            break;
        case 7:
            ioctl(isp_fd, ISP_IOC_SET_SP_LEVEL, &level);
            break;
        case 8:
            ioctl(isp_fd, ISP_IOC_SET_SP_NR_LEVEL, &level);
            break;
        case 9:
            ioctl(isp_fd, ISP_IOC_SET_SP_CLIP_LEVEL, &level);
            break;
        case 10:
            ioctl(isp_fd, ISP_IOC_SET_SAT_LEVEL, &level);
            break;
        case 99:
            return 0;
        }
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
        printf(" 8. Gamma type\n");
        printf(" 9. Gamma curve\n");
        printf("10. Auto adjustment function\n");
        printf("11. Fast tuning function\n");
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
            set_gamma_type();
            break;
        case 9:
            set_gamma_curve();
            break;
        case 10:
            auto_adjustment();
            break;
        case 11:
            fast_tuning();
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


static int adjust_sensor_exp_us(void)
{
    u32 exp_us;
    int ret;

    ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_EXP_US, &exp_us);
    if (ret < 0)
        return ret;
    printf("Exposure time = %d in unit of us\n", exp_us);

    do {
        printf("Exposure time >> ");
        exp_us = get_choose_int();
    } while (exp_us < 0);

    return ioctl(isp_fd, ISP_IOC_SET_SENSOR_EXP_US, &exp_us);
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
        printf(" 13. Sensor exposure time (us)\n");
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
        case 13:
            adjust_sensor_exp_us();
            break;
        case 99:
            return 0;
        }
    }
    return 0;
}

static int lens_get_zoom_info(void)
{
    int ret;
    zoom_info_t zoom_info;
    
    ret = ioctl(isp_fd, LENS_IOC_GET_ZOOM_INFO, &zoom_info);
    if (ret < 0){
        printf("LENS_IOC_GET_ZOOM_INFO fail!\n");
        return ret;
    }
    printf("Zoom info:\n");
    printf("    curr_zoom_index = %d\n",zoom_info.curr_zoom_index);
    printf("    curr_zoom_x10 = %d\n",zoom_info.curr_zoom_x10);
    printf("    zoom_stage_cnt = %d\n",zoom_info.zoom_stage_cnt);

    return ret;
}

static int lens_get_focus_range(void)
{
    int ret;
    focus_range_t focus_range;
    ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_RANGE, &focus_range);
    if (ret < 0){
        printf("LENS_IOC_GET_FOCUS_RANGE fail!\n");
        return ret;
    }
    printf("Focus range: %d, %d\n",focus_range.min, focus_range.max);

    return ret;
}

static int lens_set_focus_home(void)
{
    return ioctl(isp_fd, LENS_IOC_SET_FOCUS_INIT, NULL);
}

static int lens_set_focus_pos(void)
{
    int ret, curr_pos;

    if((ret = ioctl(isp_fd, LENS_IOC_GET_FOCUS_POS, &curr_pos)) < 0){
        printf("LENS_IOC_GET_FOCUS_POS fail!\n");
        return ret;
    }  
    printf("current focus pos = %d\n", curr_pos);

    do {
        printf("new focus pos >> ");
        curr_pos = get_choose_int();
    } while (0);

    return ioctl(isp_fd, LENS_IOC_SET_FOCUS_POS, &curr_pos);
}

static int lens_set_zoom_home(void)
{
    return ioctl(isp_fd, LENS_IOC_SET_ZOOM_INIT, NULL);
}

static int af_re_trigger(void);
static int lens_zoom_move(void)
{
    int tar_pos; 
    do {
        printf("new zoom x >> ");
        tar_pos = get_choose_int();
    } while (0);

    ioctl(isp_fd, LENS_IOC_SET_ZOOM_MOVE, &tar_pos);
    sleep(1);
		af_re_trigger();    

    return 0;
}

int lens_ioctl_menu(void)
{
    int option;

    while (1) {
        printf("----------------------------------------\n");
        printf(" 1.  Get zoom information\n");
        printf(" 2.  Get focus range\n");
        printf(" 3.  Set focus to initial position\n");
        printf(" 4.  Set focus position\n");
        printf(" 5.  Set zoom to initial position\n");
        printf(" 6.  Move zoom position\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");
        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {
        case 1:
            lens_get_zoom_info();
            break;
        case 2:
            lens_get_focus_range();
            break;
        case 3:
            lens_set_focus_home();
            break;
        case 4:
            lens_set_focus_pos();
            break;
        case 5:
            lens_set_zoom_home();
            break;
        case 6:
            lens_zoom_move();
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

int gm_ae_ioctl_menu(void)
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

int gm_awb_ioctl_menu(void)
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

int gm_af_ioctl_menu(void)
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

int main(int argc, char *argv[])
{
    int ret = 0;
    int option;
    u32 id, ver;

    // open ISP device
    isp_fd = open("/dev/isp328", O_RDWR);
    if (isp_fd < 0) {
        printf("Open ISP328 fail\n");
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
    if (id != 0x8136)
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
        printf(" 1. Image pipe ioctl\n");
        printf(" 2. User preference ioctl\n");
        printf(" 3. Sensor ioctl\n");
        printf(" 4. Lens ioctl\n");
        printf(" 5. Grain Media AE ioctl\n");
        printf(" 6. Grain Media AWB ioctl\n");
        printf(" 7. Grain Media AF ioctl\n");
        printf("99. Quit\n");
        printf("----------------------------------------\n");

        do {
            printf(">> ");
            option = get_choose_int();
        } while (option < 0);

        switch (option) {

        case 1:
            image_pipe_ioctl_menu();
            break;
        case 2:
            user_pref_ioctl_menu();
            break;
        case 3:
            sensor_ioctl_menu();
            break;
        case 4:
            lens_ioctl_menu();
            break;
        case 5:
            gm_ae_ioctl_menu();
            break;
        case 6:
            gm_awb_ioctl_menu();
            break;
        case 7:
            gm_af_ioctl_menu();
            break;
        case 99:
            goto end;
        }
    }

end:
    close(isp_fd);
    return 0;
}
