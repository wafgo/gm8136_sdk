/*
 * (C) Copyright 2012
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <watchdog.h>
#include <usb.h>
#include <asm/io.h>

#include "ehci.h"

unsigned int hcd_base;
unsigned int otg_base_list[CONFIG_USB_MAX_CONTROLLER_COUNT] = CONFIG_USB_EHCI_BASE_LIST;

#define OTG_READ(offset)            readl(hcd_base + offset)
#define OTG_WRITE(offset, value)    writel(value, hcd_base + offset)
#define OTG_BIT_SET(offset, bit)    writel(readl(hcd_base + offset) | (bit), hcd_base + offset)
#define OTG_BIT_CLR(offset, bit)    writel(readl(hcd_base + offset) & ~(bit), hcd_base + offset)

void pmu_write_reg(int offset, unsigned int val, unsigned int mask)
{
    unsigned int tmp, base = CONFIG_PMU_BASE + offset;

    if (val & ~mask) {
        printf("Error: value and mask not matched\n");
        for (;;) {} //bug
    }

    tmp = readl(base) & ~mask;
    tmp |= (val & mask);
    writel(tmp, base);
}

#ifdef CONFIG_PLATFORM_GM8210
void pmu_pcie_write_reg(int offset, unsigned int val, unsigned int mask)
{
    unsigned int tmp, base = CONFIG_8312_PMU_BASE + offset;

    if (val & ~mask) {
        printf("Error: value and mask not matched\n");
        for (;;) {} //bug
    }

    tmp = readl(base) & ~mask;
    tmp |= (val & mask);
    writel(tmp, base);
}
#endif

/* for FOTG210 */
int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
    int ret = 0;

    hcd_base = otg_base_list[index];

    /* init OTG controller */
#ifdef CONFIG_PLATFORM_GM8210
    switch (hcd_base) {
        case CONFIG_FOTG210_0_BASE:
            /* Set OTG0 H2X register */
            writel(readl(CONFIG_FOTG210_0_H2X_BASE) & ~BIT3, CONFIG_FOTG210_0_H2X_BASE);
            writel(readl(CONFIG_FOTG210_0_H2X_BASE) | BIT2, CONFIG_FOTG210_0_H2X_BASE);
            //======= OTG0 Part======
            /* BIT2:pllaliv; BIT3:oscouten; BIT8:0=external clock, 1=internal clock */
            pmu_pcie_write_reg(0x84, BIT2|BIT3|BIT8, BIT2|BIT3|BIT8);
            /* OTG0 PHY coreclkin. 0: on, 1:off */
            pmu_pcie_write_reg(0x84, 0, BIT27);
            /* Disable OTG0 phy POR */
            pmu_pcie_write_reg(0x84, BIT4, BIT4);
            /* Configure OTG0 in host mode */
            pmu_pcie_write_reg(0x84, 0, BIT0);
            /* Enable OTG0 VBUS input */
            pmu_pcie_write_reg(0x84, BIT1, BIT1);
            /* Turn on OTG0 clock */
            pmu_pcie_write_reg(0x30, 0, BIT6);
            break;
        case CONFIG_FOTG210_1_BASE:
            /* Set OTG1 H2X register */
            writel(readl(CONFIG_FOTG210_1_H2X_BASE) & ~BIT3, CONFIG_FOTG210_1_H2X_BASE);
            writel(readl(CONFIG_FOTG210_1_H2X_BASE) | BIT2, CONFIG_FOTG210_1_H2X_BASE);
            //======= OTG1 Part======
            /* BIT11:pllaliv; BIT12:oscouten; BIT17:0=external clock, 1=internal clock */
            pmu_pcie_write_reg(0x84, BIT11|BIT12|BIT17, BIT11|BIT12|BIT17);
            /* OTG1 PHY coreclkin. 0: on, 1:off */
            pmu_pcie_write_reg(0x84, 0, BIT28);
            /* Disable OTG1 phy POR */
            pmu_pcie_write_reg(0x84, BIT13, BIT13);
            /* Configure OTG1 in host mode */
            pmu_pcie_write_reg(0x84, 0, BIT9);
            /* Enable OTG1 VBUS input */
            pmu_pcie_write_reg(0x84, BIT10, BIT10);
            /* Turn on OTG1 clock */
            pmu_pcie_write_reg(0x30, 0, BIT5);
            break;
        case CONFIG_FOTG210_2_BASE:
            /* Set OTG2 H2X register */
            writel(readl(CONFIG_FOTG210_2_H2X_BASE) & ~BIT3, CONFIG_FOTG210_2_H2X_BASE);
            writel(readl(CONFIG_FOTG210_2_H2X_BASE) | BIT2, CONFIG_FOTG210_2_H2X_BASE);
            //======= OTG2(PHY 1.1) Part======
            /* Configure OTG2 in host mode */
            pmu_pcie_write_reg(0x84, 0, BIT18);
            /* Enable OTG2 VBUS input */
            pmu_pcie_write_reg(0x84, BIT19, BIT19);
            /* Turn on OTG2 clock */
            pmu_pcie_write_reg(0x30, 0, BIT4);
            /* force to full-speed */
            OTG_BIT_SET(0x80, BIT12);
            break;
        default:
            printf("\nError: Invalid OTG base address\n");
            ret = -1;
            goto exit;
            break;
    }
#endif
#ifdef CONFIG_PLATFORM_GM8287
    switch (hcd_base) {
        case CONFIG_FOTG210_0_BASE:
            pmu_write_reg(0xC0, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            pmu_write_reg(0xC0, 0, BIT0); /* OTG0 PHY clock */
            pmu_write_reg(0xC0, BIT6, BIT6);
            pmu_write_reg(0xC0, BIT3, BIT3);
            pmu_write_reg(0xC0, 0, BIT2);
            pmu_write_reg(0xC0, 0, BIT1); /* OTG0 hclk on */
            break;
        case CONFIG_FOTG210_1_BASE:
            pmu_write_reg(0xC8, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            pmu_write_reg(0xC8, 0, BIT0); /* OTG1 PHY clock */
            pmu_write_reg(0xC8, BIT6, BIT6);
            pmu_write_reg(0xC8, BIT3, BIT3);
            pmu_write_reg(0xC8, 0, BIT2);
            pmu_write_reg(0xC8, 0, BIT1); /* OTG1 hclk on */
            break;
        default:
            printf("\nError: Invalid OTG base address\n");
            ret = -1;
            goto exit;
            break;
    }
#endif
#ifdef CONFIG_PLATFORM_GM8139
    switch (hcd_base) {
        case CONFIG_FOTG210_0_BASE:
            pmu_write_reg(0xC0, BIT24 | BIT25, BIT24 | BIT25 | BIT26 | BIT27);
            pmu_write_reg(0xC0, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            pmu_write_reg(0xAC, 0, BIT0); /* OTG0 PHY clock */
            pmu_write_reg(0xC0, BIT6, BIT6);
            pmu_write_reg(0xC0, BIT3, BIT3);
            pmu_write_reg(0xC0, 0, BIT2);
            pmu_write_reg(0xB4, 0, BIT9); /* OTG0 hclk on */
            break;
        default:
            printf("\nError: Invalid OTG base address\n");
            ret = -1;
            goto exit;
            break;
    }
#endif
#ifdef CONFIG_PLATFORM_GM8136
    switch (hcd_base) {
        case CONFIG_FOTG210_0_BASE:
            pmu_write_reg(0xC0, BIT24 | BIT25, BIT24 | BIT25 | BIT26 | BIT27);
            pmu_write_reg(0xC0, BIT4 | BIT5 | BIT19, BIT4 | BIT5 | BIT19);
            pmu_write_reg(0xAC, 0, BIT0); /* OTG0 PHY clock */
            pmu_write_reg(0xC0, BIT6, BIT6);
            pmu_write_reg(0xC0, BIT3, BIT3);
            pmu_write_reg(0xC0, 0, BIT2);
            pmu_write_reg(0xB4, 0, BIT9); /* OTG0 hclk on */
            break;
        default:
            printf("\nError: Invalid OTG base address\n");
            ret = -1;
            goto exit;
            break;
    }
#endif

	mdelay(500); // waiting for PHY clock be stable, while clock source changed from externel to internel, at lease 5ms

    if ((OTG_READ(0x80) & BIT21) == 0) {
        //printf("Enter Device A\n");
        OTG_WRITE(0x84, OTG_READ(0x84));
        OTG_BIT_SET(0x80, BIT5);
        OTG_BIT_CLR(0x80, BIT4);
        mdelay(10);
        //printf("Drive Vbus because of ID pin shows Device A\n");
        OTG_BIT_CLR(0x80, BIT5);
        OTG_BIT_SET(0x80, BIT4);
        mdelay(10);
    } else {
        printf("\nError: OTG controller init failed\n");
        ret = -1;
        goto exit;
    }

    *hccr = (struct ehci_hccr *)hcd_base;
    *hcor = (struct ehci_hcor *)((uint32_t)*hccr +
            HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

exit:
	return ret;
}

int ehci_hcd_stop(int index)
{
	return 0;
}
