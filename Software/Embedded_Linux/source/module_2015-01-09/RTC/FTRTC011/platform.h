#ifndef _RTC_PLATFORM_H_
#define _RTC_PLATFORM_H_

/*************************************************************************************
 *  Platform Global Definition
 *************************************************************************************/
typedef enum {
    PLAT_RTC_CLK_SRC_OSC = 0,
    PLAT_RTC_CLK_SRC_PLL3,
    PLAT_RTC_CLK_SRC_MAX
} PLAT_RTC_CLK_SRC_T;

/*************************************************************************************
 *  Platform Public Function Prototype
 *************************************************************************************/
void rtc_plat_identifier_check(void);
int  rtc_plat_pmu_init(void);
void rtc_plat_pmu_exit(void);
int  rtc_plat_rtc_clk_gate(int onoff);
int  rtc_plat_rtc_clk_set_src(PLAT_RTC_CLK_SRC_T clk_src);

#endif  /* _RTC_PLATFORM_H_ */
