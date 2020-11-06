/**
 * @file spi_common.c
 * SPI for sensor source code
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 */

#include "isp_dbg.h"
#include "spi_common.h"

static int isp_spi_pmu_fd = 0;

static pmuReg_t isp_spi_pmu_reg_8139[] = {
    /*
     * Multi-Function Port Setting Register 1 [offset=0x54]
     * ----------------------------------------------------------------
     *  [21:20] 0:GPIO_0[17],   1:CAP0_D0,  2:x         3:SSP1_SCLK
     *  [19:18] 0:GPIO_0[16],   1:CAP0_D1,  2:x         3:SSP1_TXD
     *  [17:16] 0:GPIO_0[15],   1:CAP0_D2,  2:I2C_SDA   3:SSP1_RXD
     *  [15:14] 0:GPIO_0[14],   1:CAP0_D3,  2:I2C_SCL   3:SSP1_FS
     */
    {
     .reg_off   = 0x54,
     .bits_mask = 0x003fc000,
     .lock_bits = 0x003fc000,
     .init_val  = 0x00000000,
     .init_mask = 0x003fc000,
    },
};

static pmuRegInfo_t isp_spi_pmu_reg_info_8139 = {
    "ISP_SPI_8139",
    ARRAY_SIZE(isp_spi_pmu_reg_8139),
    ATTR_TYPE_AHB,
    &isp_spi_pmu_reg_8139[0]
};

int isp_api_spi_init(void)
{
    int fd, ret;

    // request GPIO
    ret = gpio_request(SPI_CLK, "SPI_CLK");
    if (ret < 0)
        return ret;
    ret = gpio_request(SPI_DI, "SPI_DI");
    if (ret < 0)
        goto spi_di_err;
    ret = gpio_request(SPI_DO, "SPI_DO");
    if (ret < 0)
        goto spi_do_err;
    ret = gpio_request(SPI_CS, "SPI_CS");
    if (ret < 0)
        goto spi_cs_err;

    // set GPIO direction
    gpio_direction_output(SPI_CLK, 1);
    gpio_direction_input(SPI_DI);
    gpio_direction_output(SPI_DO, 1);
    gpio_direction_output(SPI_CS, 1);

    // set pinmux
    if ((fd = ftpmu010_register_reg(&isp_spi_pmu_reg_info_8139)) < 0) {
        isp_err("register pmu failed!!\n");
        ret = -1;
        goto reg_pmu_err;
    }
    isp_spi_pmu_fd = fd;

    return 0;

reg_pmu_err:
    gpio_free(SPI_CS);
spi_cs_err:
    gpio_free(SPI_DO);
spi_do_err:
    gpio_free(SPI_DI);
spi_di_err:
    gpio_free(SPI_CLK);

    return ret;
}

void isp_api_spi_exit(void)
{
    // release GPIO
    gpio_free(SPI_CLK);
    gpio_free(SPI_DI);
    gpio_free(SPI_DO);
    gpio_free(SPI_CS);

    if (isp_spi_pmu_fd) {
        ftpmu010_deregister_reg(isp_spi_pmu_fd);
        isp_spi_pmu_fd = 0;
    }
}

void isp_api_spi_clk_set_value(int value)
{
    gpio_set_value(SPI_CLK, value);
}

int isp_api_spi_di_get_value(void)
{
    return (gpio_get_value(SPI_DI) >> PIN_NUM(SPI_DI));
}

void isp_api_spi_do_set_value(int value)
{
    gpio_set_value(SPI_DO, value);
}

void isp_api_spi_cs_set_value(int value)
{
    gpio_set_value(SPI_CS, value);
}
