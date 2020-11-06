/**
 * @file isp_api.h
 * This file provides the isp320 driver related function calls or structures
 *
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef _ISP_API_H_
#define _ISP_API_H_

#include <linux/types.h>

/*************************************************************************************
 * ISP API Interface Version
 *************************************************************************************/
#define ISP_API_MAJOR_VER       0
#define ISP_API_MINOR_VER       1
#define ISP_API_BETA_VER        1
#define ISP_API_VERSION         (((ISP_API_MAJOR_VER)<<16)|((ISP_API_MINOR_VER)<<8)|(ISP_API_BETA_VER))  /* 0.1.0 */

/*************************************************************************************
 * ISP API Parmaeter Definition
 *************************************************************************************/
typedef struct isp_info {
    int cap_width;      // ISP output image width
    int cap_height;     // ISP output image height
    int fps;            // frame rate
    int max_fps;        // maximum frame rate
} isp_info_t;

/*************************************************************************************
 * ISP API Export Function Prototype
 *************************************************************************************/
/*
 * @This function used to get isp export API version code, for compability check
 *
 * @function u32 isp_api_get_version(void)
 * @return version code
 */
u32 isp_api_get_version(void);

/*
 * @This function used to start ISP
 *
 * @function int isp_api_start_isp(void)
 * @return 0 on success, <0 on error
 */
int isp_api_start_isp(void);

/*
 * @This function used to stop ISP
 *
 * @function int isp_api_stop_isp(void)
 * @return 0 on success, <0 on error
 */
int isp_api_stop_isp(void);

/*
 * @This function used to get ISP infomation
 *
 * @function int isp_api_get_info(struct isp_info *pinfo)
 * @param pinfo, pointer to struct isp_info data
 * @return 0 on success, <0 on error
 */
int isp_api_get_info(struct isp_info *pinfo);

/*
 * @This function used to set ISP output image size
 *
 * @function int isp_api_set_cap_size(int width, int height)
 * @param width, isp output image width
 * @param height, isp output image height
 * @return 0 on success, <0 on error
 */
int isp_api_set_cap_size(int width, int height);

/*
 * @This function used to notify ISP that pclk is stop
 *
 * @function int isp_api_notice_no_pclk(void)
 */
void isp_api_notice_no_pclk(void);

/*
 * @This function used to set EXT_CLK frequency
 *
 * @function int isp_api_extclk_set_freq(u32 freq)
 * @param freq, EXT_CLK frequency
 * @return 0 on success, <0 on error
 */
int isp_api_extclk_set_freq(u32 freq);

/*
 * @This function used to turn on/off EXT_CLK
 *
 * @function int isp_api_extclk_onoff(int on)
 * @param on, 0:turn off EXT_CLK, 1:turn on EXT_CLK
 * @return 0 on success, <0 on error
 */
int isp_api_extclk_onoff(int on);

/*
 * @This function used to initial CAP_RST pin
 *
 * @function int isp_api_cap_rst_init(void)
 * @return 0 on success, <0 on error
 */
int isp_api_cap_rst_init(void);

/*
 * @This function used to release CAP_RST pin
 *
 * @function void isp_api_cap_rst_exit(void)
 */
void isp_api_cap_rst_exit(void);

/*
 * @This function used set or clear CAP_RST pin
 *
 * @function void isp_api_cap_rst_set_value(int value)
 * @param value, 0 or 1
 */
void isp_api_cap_rst_set_value(int value);

#endif  /* _ISP_API_H_ */
