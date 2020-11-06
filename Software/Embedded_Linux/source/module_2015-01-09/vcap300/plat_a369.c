/**
 * @file plat_a369.c
 *  vcap300 platform releated api for A369
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.2 $
 * $Date: 2014/05/22 06:14:47 $
 *
 * ChangeLog:
 *  $Log: plat_a369.c,v $
 *  Revision 1.2  2014/05/22 06:14:47  jerson_l
 *  1. add get platfrom chip version api.
 *
 *  Revision 1.1  2013/01/15 09:34:09  jerson_l
 *  1. add A369 platform releated control api
 *
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "bits.h"
#include "vcap_dev.h"
#include "vcap_dbg.h"
#include "vcap_plat.h"

int vcap_plat_pmu_init(void)
{
    return 0;
}

void vcap_plat_pmu_exit(void)
{
    return;
}

u32 vcap_plat_chip_version(void)
{
    return 0;
}

int vcap_plat_cap_data_swap(int cap_id, int bit_swap, int byte_swap)
{
    return 0;
}

int vcap_plat_cap_data_duplicate(int cap_id, int dup_on)
{
    return 0;
}

int vcap_plat_cap_clk_invert(int cap_id, int clk_invert)
{
    return 0;
}

int vcap_plat_cap_pin_mux(int cap_id, VCAP_PLAT_CAP_PINMUX_T cap_pin)
{
    return 0;
}

int vcap_plat_cap_schmitt_trigger_onoff(int on)
{
    return 0;
}

int vcap_plat_cap_sw_reset_onoff(int on)
{
    return 0;
}

int vcap_plat_cap_axiclk_onoff(int on)
{
    return 0;
}

int vcap_plat_vi_capture_port_config(int vi, struct vcap_plat_cap_port_cfg_t *cfg)
{
    return 0;
}
