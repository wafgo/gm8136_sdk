/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>

#ifdef CONFIG_CMD_GO

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec(ulong (*entry)(int, char * const []), int argc,
				 char * const argv[])
{
	return entry (argc, argv);
}

static int do_go(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CONFIG_SYS_MAXARGS, 1,	do_go,
	"start application at address 'addr'",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments"
);

#endif

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"Perform RESET of the CPU",
	""
);

#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8139)  //CPU 726
extern void set_write_alloc(u32 setting);
extern u32 get_write_alloc(void);
static int set_wa(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32    mode = 0;
	    
	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	mode = simple_strtoul(argv[1], NULL, 10);
  if(mode){
	    mode = get_write_alloc() | (1 << 20);
	}else{
	    mode = get_write_alloc() & (~(1 << 20));
	}
	set_write_alloc(mode);
	
	return 0;
}

U_BOOT_CMD(
	setwa, 2, 1,	set_wa,
	"Set write allocate, 1 is on, 0 is off\n",
	""
);

static int get_wa(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 1) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	printf("Debug-Override register data = 0x%x\n", get_write_alloc());
	
	return 0;
}

U_BOOT_CMD(
	getwa, 1, 0,	get_wa,
	"Read Debug-Override register data\n",
	""
);
#endif

#ifdef CONFIG_USE_IRQ
extern void display_sys_freq(void);

#ifdef CONFIG_PLATFORM_GM8126
extern void FCS_go(unsigned int cpu3x, unsigned int mul, unsigned int div);
extern void set_PLL2(uint pll2_mul, uint pll2_div);
static int do_fcs (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint    pll1_mul = 0, pll1_div = 0, pll2_mul = 0, pll2_div = 0;
	int     rcode = 0;

	if (argc < 5) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	pll1_mul = simple_strtoul(argv[1], NULL, 10);
	pll1_div = simple_strtoul(argv[2], NULL, 10);
	pll2_mul = simple_strtoul(argv[3], NULL, 10);
	pll2_div = simple_strtoul(argv[4], NULL, 10);

    //printf ("data:%d,%d,%d,%d\n", pll1_mul, pll1_div, pll2_mul, pll2_div);
	set_PLL2(pll2_mul, pll2_div);
	FCS_go(444, pll1_mul, pll1_div);

    display_sys_freq();
	return rcode;
}
//fcs 80 3 54 3 =>8126
//fcs 90 3 99 5 =>8128
U_BOOT_CMD(
	fcs,	5,	1,	do_fcs,
	"frequency change sequence\n",
	"[PLL1 mul] [PLL1 div] [PLL2 mul] [PLL2 div]	- set FCS\n"
);

#else	
extern void FCS_go(unsigned int cpu3x, unsigned int pll1_mul, unsigned int pll4_mul);
extern uint u32PMU_ReadPLL1CLK(void);
static int do_fcs (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#if 0    
	uint    cpu_mode = 0, pll1_mul = 0, pll4_mul = 0;
#else
	uint    pll1_mul = 0, pll4_mul = 0;
#endif	
	int     rcode = 0;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
#if 0	
	cpu_mode = simple_strtoul(argv[1], NULL, 10);
	pll1_mul = simple_strtoul(argv[2], NULL, 10);
	pll4_mul = simple_strtoul(argv[3], NULL, 10);
#else
	pll1_mul = u32PMU_ReadPLL1CLK() / SYS_CLK;
	pll4_mul = simple_strtoul(argv[1], NULL, 10);
#endif	
    //printf ("data:%d,%d,%d\n",cpu_mode, pll1_mul, pll1_div);
	//FCS_go(cpu_mode, pll1_mul, pll4_mul);
	//FCS_go(0x286D, 99, pll4_mul);
	FCS_go(0x29ED, pll1_mul, pll4_mul);
	//FCS_go(0x21E5, 83, pll4_mul);

    display_sys_freq();
	return rcode;
}

U_BOOT_CMD(
	fcs,	2,	1,	do_fcs,
	"frequency change sequence\n",
	//"[CPU mode] [PLL1 mul] [PLL4 mul] - set FCS\n"
	"[PLL4 mul] - set FCS\n" 
);
#endif
#endif

#ifdef CONFIG_PLATFORM_GM8210
extern int do_loadsl (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

U_BOOT_CMD(
	loadsl,	3,	1,	do_loadsl,
	"load linux image from host side to slave side",
	"local_address image_size"
);
#endif
