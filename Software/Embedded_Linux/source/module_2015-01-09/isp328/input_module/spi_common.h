/**
 * @file  spi_common.h
 * SPI for sensor header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __SPI_COMMON_H__
#define __SPI_COMMON_H__

#include <linux/gpio.h>
#include <mach/ftpmu010.h>

//============================================================================
// struct & flag definition
//============================================================================
#define GPIO_ID(group, pin)     ((32*group)+pin)
#define PIN_NUM(id)             (id%32)

#define CAP_RST     GPIO_ID(0,  5)
#define SPI_CLK     GPIO_ID(0, 17)
#define SPI_DI      GPIO_ID(0, 15)
#define SPI_DO      GPIO_ID(0, 16)
#define SPI_CS      GPIO_ID(0, 14)

//============================================================================
// external functions
//============================================================================
/*
 * @This function used to initial ISP GPIO SPI
 *
 * @function int isp_api_spi_init(void)
 * @return 0 on success, <0 on error
 */
int isp_api_spi_init(void);

/*
 * @This function used to release ISP GPIO SPI
 *
 * @function void isp_api_spi_exit(void)
 */
void isp_api_spi_exit(void);

/*
 * @This function used set or clear SPI_CLK pin
 *
 * @function void isp_api_spi_clk_set_value(int value)
 * @param value, 0 or 1
 */
void isp_api_spi_clk_set_value(int value);

/*
 * @This function used read SPI_DI status
 *
 * @function int isp_api_spi_di_get_value(void)
   @return 0 or 1
 */
int isp_api_spi_di_get_value(void);

/*
 * @This function used set or clear SPI_DO pin
 *
 * @function void isp_api_spi_do_set_value(int value)
 * @param value, 0 or 1
 */
void isp_api_spi_do_set_value(int value);

/*
 * @This function used set or clear SPI_CS pin
 *
 * @function void isp_api_spi_cs_set_value(int value)
 * @param value, 0 or 1
 */
void isp_api_spi_cs_set_value(int value);


#endif /*__SPI_COMMON_H__*/
