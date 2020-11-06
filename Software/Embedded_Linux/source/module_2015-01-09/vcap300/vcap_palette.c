/**
 * @file vcap_palette.c
 *  vcap300 palette control
 * Copyright (C) 2012 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.3 $
 * $Date: 2012/12/11 09:15:20 $
 *
 * ChangeLog:
 *  $Log: vcap_palette.c,v $
 *  Revision 1.3  2012/12/11 09:15:20  jerson_l
 *  1. modify vcap_palette_get api prototype
 *
 *  Revision 1.2  2012/10/24 06:47:51  jerson_l
 *
 *  switch file to unix format
 *
 *  Revision 1.1.1.1  2012/10/23 08:16:31  jerson_l
 *  import vcap300 driver
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "vcap_dev.h"
#include "vcap_palette.h"
#include "vcap_dbg.h"

static const u32 vcap_palette_default[VCAP_PALETTE_MAX] = {
    VCAP_PALETTE_COLOR_WHITE,
    VCAP_PALETTE_COLOR_BLACK,
    VCAP_PALETTE_COLOR_RED,
    VCAP_PALETTE_COLOR_BLUE,
    VCAP_PALETTE_COLOR_YELLOW,
    VCAP_PALETTE_COLOR_GREEN,
    VCAP_PALETTE_COLOR_BROWN,
    VCAP_PALETTE_COLOR_DODGERBLUE,
    VCAP_PALETTE_COLOR_GRAY,
    VCAP_PALETTE_COLOR_KHAKI,
    VCAP_PALETTE_COLOR_LIGHTGREEN,
    VCAP_PALETTE_COLOR_MAGENTA,
    VCAP_PALETTE_COLOR_ORANGE,
    VCAP_PALETTE_COLOR_PINK,
    VCAP_PALETTE_COLOR_SLATEBLUE,
    VCAP_PALETTE_COLOR_AQUA
};

int vcap_palette_init(struct vcap_dev_info_t *pdev_info)
{
    int i;

    for(i=0; i<VCAP_PALETTE_MAX; i++)
        VCAP_REG_WR(pdev_info, VCAP_PALETTE_OFFSET(i*4), vcap_palette_default[i]);

    return 0;
}

int vcap_palette_set(struct vcap_dev_info_t *pdev_info, int id, u32 crcby)
{
    if(id >= VCAP_PALETTE_MAX)
        return -1;

    VCAP_REG_WR(pdev_info, VCAP_PALETTE_OFFSET(id*4), crcby);

    return 0;
}

int vcap_palette_get(struct vcap_dev_info_t *pdev_info, int id, u32 *crcby)
{
    if(id >= VCAP_PALETTE_MAX || !crcby)
        return -1;

    *crcby = VCAP_REG_RD(pdev_info, VCAP_PALETTE_OFFSET(id*4));

    return 0;
}
