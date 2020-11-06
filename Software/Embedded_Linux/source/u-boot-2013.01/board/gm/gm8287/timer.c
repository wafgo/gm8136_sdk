/*
 * (C) Copyright 2009 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <div64.h>
#include <asm/io.h>

//#define TIMER_LOAD_VAL	135000000  //APB clock u32PMU_ReadAPB0CLK
#define CFG_HZ              1000

extern unsigned int u32PMU_ReadAPB0CLK(void);
/* The Integrator/CP timer1 is clocked at 1MHz
 * can be divided by 16 or 256
 * and can be set up as a 32-bit timer
 */
/* U-Boot expects a 32 bit timer, running at CFG_HZ */
/* Keep total timer count to avoid losing decrements < div_timer */
static unsigned long long total_count = 0;
static unsigned long long lastdec;	 /* Timer reading at last call */
static unsigned long long div_clock = 1; /* Divisor applied to timer clock */
static unsigned long long div_timer = 1; /* Divisor to convert timer reading
					  * change to U-Boot ticks
					  */
/* CFG_HZ = CFG_HZ_CLOCK/(div_clock * div_timer) */
static ulong timestamp;		/* U-Boot ticks since startup */

/* converts the timer reading to U-Boot ticks */
/* the timestamp is the number of ticks since reset */
ulong get_timer_masked (void)
{
	/* get current count */
	unsigned long long now = readl(CONFIG_TMR_BASE + 0);

	if(now > lastdec) {
		/* Must have wrapped */
		total_count += lastdec + u32PMU_ReadAPB0CLK() + 1 - now;
	} else {
		total_count += lastdec - now;
	}
	lastdec = now;
	timestamp = (ulong)(total_count/div_timer); /* convert to u-boot ticks */
//printf("<%x>",timestamp);
	return timestamp;
}

void reset_timer_masked (void)
{
	/* capure current decrementer value */
	lastdec		= readl(CONFIG_TMR_BASE + 0);
	/* start "advancing" time stamp from 0 */
	timestamp = 0L;
}

int timer_init(void)
{
	/* Load timer with initial value */
	writel(u32PMU_ReadAPB0CLK(), CONFIG_TMR_BASE);
	/* Set timer to be
	 *	enabled			1
	 *	periodic		1
	 *	no interrupts		0
	 *	X			0
	 *	divider 1	 00 == less rounding error
	 *	32 bit			1
	 *	wrapping		0
	 */
//	writel(CONFIG_TMR_BASE + 8) = 0x000000C2;
	writel(u32PMU_ReadAPB0CLK(), CONFIG_TMR_BASE + 4);//0xffffffff;
	writel(0, CONFIG_TMR_BASE + 8);
	writel(0x00000001, CONFIG_TMR_BASE + 0x30);
	/* init the timestamp */
	total_count = 0ULL;
	reset_timer_masked();

	div_timer	= (unsigned long long)(u32PMU_ReadAPB0CLK() / CFG_HZ); /* each u-boot tick need how much timer tick */
	div_timer /= div_clock;

	return 0;
}

/*
 * Get the current 64 bit timer tick count
 */
unsigned long long get_ticks(void)
{
	return (unsigned long long)get_timer(0);
}

void __udelay(unsigned long usec)
{
	ulong tmo, tmp;

	/* Convert to U-Boot ticks */
//	tmo	= usec * CFG_HZ;
//	tmo /= (1000000L);
	tmo	= usec * (u32PMU_ReadAPB0CLK() /1000000L);  /* need how much timer ticks */
        tmo/=div_timer; /* need how much u-boot ticks */

	tmp = get_timer_masked();	/* get current timestamp */
	tmo += tmp;			/* form target timestamp */

	while (get_timer_masked () < tmo) {/* loop till event */
		/*NOP*/;
	}
}

/*
 * get_timer(base) can be used to check for timeouts or
 * to measure elasped time relative to an event:
 *
 * ulong start_time = get_timer(0) sets start_time to the current
 * time value.
 * get_timer(start_time) returns the time elapsed since then.
 *
 * The time is used in CONFIG_SYS_HZ units!
 */
ulong get_timer(ulong base)
{
	return get_timer_masked () - base;
}

/*
 * Return the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return (ulong)((unsigned long long)CFG_HZ);
}
