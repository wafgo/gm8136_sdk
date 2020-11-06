/*
 * (C) Copyright 2003-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2004
 * Martin Krause, TQ-Systems GmbH, martin.krause@tqs.de
 *
 * Modified for the CMC PU2 by (C) Copyright 2004 Gary Jennejohn
 * garyj@denx.de
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

#include <common.h>
#include "flash_new.h"

#ifndef	CONFIG_ENV_ADDR
#define CONFIG_ENV_ADDR	(CFG_FLASH_BASE + CONFIG_ENV_OFFSET)
#endif

flash_info_t	flash_info[CFG_MAX_FLASH_BANKS]; /* info for FLASH chips */

//for 8 bit
#define FLASH_CYCLE1	0xAAA
#define FLASH_CYCLE2	0x555

//for 16bit
#define FLASH_HW_CYCLE1	0x0555
#define FLASH_HW_CYCLE2	0x02AA

/*-----------------------------------------------------------------------
 * Functions
 */
static ulong flash_get_size(vu_short *addr, flash_info_t *info);
static void flash_reset(flash_info_t *info);
static int write_word_amd(flash_info_t *info, vu_short *dest, ushort data);
static int write_buffer_nmx (flash_info_t *info, vu_char *dest, uchar *src); 	// added by Numonyx R.K. 2009/06/05
static flash_info_t *flash_get_info(ulong base);

/*-----------------------------------------------------------------------
 * flash_init()
 *
 * sets up flash_info and returns size of FLASH (bytes)
 */
unsigned long flash_init (void)
{
	unsigned long size = 0, reg = 0;
	ulong flashbase = CFG_FLASH_BASE;

	/* Check for SMC function */
	reg = *(volatile ulong *)(CONFIG_PMU_BASE + 0x5C);
	if(reg & (1 << 24)) {
			printf("Not for SMC pin mux\n");
			return 1;
	}

	/* Init: no FLASHes known */
	memset(&flash_info[0], 0, sizeof(flash_info_t));

	flash_info[0].size = flash_get_size((vu_short *)flashbase, &flash_info[0]);

	size = flash_info[0].size;

#ifdef	CONFIG_ENV_IS_IN_FLASH
	/* ENV protection ON by default */
	flash_protect(FLAG_PROTECT_SET,
		      CONFIG_ENV_ADDR,
		      CONFIG_ENV_ADDR+CONFIG_ENV_SIZE-1,
		      flash_get_info(CONFIG_ENV_ADDR));
#endif

	return size ? size : 1;
}

/*-----------------------------------------------------------------------
 */
static void flash_reset(flash_info_t *info)
{
	vu_short *base = (vu_short *)(info->start[0]);

	/* Put FLASH back in read mode */
	if ((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_INTEL)
		*base = 0x00FF;	/* Intel Read Mode */
	else if ((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_AMD)
		*base = 0x00F0;	/* AMD Read Mode */
}

/*-----------------------------------------------------------------------
 */

static flash_info_t *flash_get_info(ulong base)
{
	int i;
	flash_info_t * info;

	info = NULL;
	for (i = 0; i < CFG_MAX_FLASH_BANKS; i ++) {
		info = & flash_info[i];
		if (info->size && info->start[0] <= base &&
		    base <= info->start[0] + info->size - 1)
			break;
	}

	return i == CFG_MAX_FLASH_BANKS ? 0 : info;
}

/*-----------------------------------------------------------------------
 */

void flash_print_info (flash_info_t *info)
{
	int i;

	if (info->flash_id == FLASH_UNKNOWN) {
		printf ("missing or unknown FLASH type\n");
		return;
	}

	switch (info->flash_id & FLASH_VENDMASK) {
	case FLASH_MAN_AMD:	printf ("AMD ");		break;
	case FLASH_MAN_BM:	printf ("BRIGHT MICRO ");	break;
	case FLASH_MAN_FUJ:	printf ("FUJITSU ");		break;
	case FLASH_MAN_SST:	printf ("SST ");		break;
	case FLASH_MAN_STM:	printf ("STM ");		break;
	case FLASH_MAN_INTEL:	printf ("INTEL ");		break;
	default:		printf ("Unknown Vendor ");	break;
	}

	switch (info->flash_id & FLASH_TYPEMASK) {
	case FLASH_S29GL128N:
		printf ("S29GL128N (128Mbit, uniform sector size)\n");
		break;
	case FLASH_S29GL256N:
		printf ("S29GL256 (256Mbit, uniform sector size)\n");
		break;
	case FLASH_S29GL512N:
		printf ("S29GL512 (512Mbit, uniform sector size)\n");
		break;
	case FLASH_S29GL1G:
		printf ("S29GL1G (1GMbit, uniform sector size)\n");
		break;
	default:
		printf ("Unknown Chip Type\n");
		break;
	}

	printf ("  Size: %ld MB in %d Sectors\n",
		info->size >> 20,
		info->sector_count);

	printf ("  Sector Start Addresses:");

	for (i=0; i<info->sector_count; ++i) {
		if ((i % 5) == 0) {
			printf ("\n   ");
		}
		printf (" %08lX%s",
			info->start[i],
			info->protect[i] ? " (RO)" : "     ");
	}
	printf ("\n");
	return;
}

/*-----------------------------------------------------------------------
 */

/*
 * The following code cannot be run from FLASH!
 */

ulong flash_get_size (vu_short *addr, flash_info_t *info)
{
	int i;
	ushort ManID;		// Renamed by Numonyx R.K. 2009/06/06
	ushort DevID;		// Added by Numonyx R.K. 2009/06/06
	ushort DevID2;		// Added by Numonyx R.K. 2009/06/06

	ulong base = (ulong)addr;
	vu_char *dd;

    dd = (vu_char *)addr;
//printf("d=%x,d=%x\n",dd[0],dd[1]);
	/* Write auto select command sequence */
	dd[FLASH_CYCLE1] = 0xAA;	/* for AMD, Intel ignores this */
	dd[FLASH_CYCLE2] = 0x55;	/* for AMD, Intel ignores this */
	dd[FLASH_CYCLE1] = 0x90;	/* selects Intel or AMD */
	/* read Manufacturer ID */
	udelay(100);


	// Assuming 16-bit bus access. This version not defined for 8-bit
	// --------------------------------------------------------------
	ManID = dd[0x00];				// Changed by Numonyx R.K. 2009/06/06
	DevID = dd[0x02];				// Added by Numonyx R.K. 2009/06/06
	DevID2 = dd[0x1C];				// Added by Numonyx R.K. 2009/06/06


	printf ("Manufacturer ID : %04X\n", ManID);		// Changed by Numonyx R.K. 2009/06/06
	printf ("Device ID       : %04X\n", DevID);		// Added by Numonyx R.K. 2009/06/06
	printf ("Device Code 2   : %04X\n", DevID2); 	// Added by Numonyx R.K. 2009/06/06


	switch (ManID) {

	case (AMD_MANUFACT & 0xFF):
		debug ("Manufacturer: AMD (Spansion)\n");
		info->flash_id = FLASH_MAN_AMD;
		break;

	case (INTEL_MANUFACT & 0xFF):
		// Make sure differenturate Numonyx Flash
		// --------------------------------------
		if (DevID == FLASH_NMX)						// Added by Numonyx R.K. 2009/06/06
		{
			debug ("Manfuacturer: Numonyx\n");		// Added by Numonyx R.K. 2009/06/06
			info->flash_id = FLASH_NMX;				// Added by Numonyx R.K. 2009/06/06
		}
		else
		{
			debug ("Manufacturer: Intel (not supported yet)\n");
			info->flash_id = FLASH_MAN_INTEL;
		}
		break;

	case (STM_MANUFACT & 0xFF)://fix M29W128G ID issue
	case (MX_MANUFACT & 0xFF):
		debug ("Manufacturer: MXIC\n");
		info->flash_id = FLASH_MAN_AMD;
		break;

	case (EON_MANUFACT & 0xFF):
		debug ("Manufacturer: EON\n");
		info->flash_id = FLASH_MAN_AMD;
		break;

//	case (NUM_MANUFACT & 0xFF):						// removed by Numonyx R.K. 2009/06/06
//		debug ("Manufacturer: Numonyx\n");			// removed by Numonyx R.K. 2009/06/06
//		info->flash_id = FLASH_MAN_AMD;				// removed by Numonyx R.K. 2009/06/06
//		break;										// removed by Numonyx R.K. 2009/06/06

	default:
		//printf ("Unknown Manufacturer ID: %04X\n", value);
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->size = 0;
		goto out;
	}

//	value = dd[2];									// removed by Numonyx R.K. 2009/06/06
//	printf ("Device ID	: %04X\n", value);			// removed by Numonyx R.K. 2009/06/08

	switch (DevID)
	{

	case (AMD_ID_MIRROR & 0xFF):
		if (ManID == INTEL_MANUFACT)								// add by Numonyx R.K. 2009/06/08
		{
			printf ("Numonyx Axcell(R) MLC flash: %04X\n", DevID2); // add by Numonyx R.K. 2009/06/08
			switch(DevID2)
			{											// changed by Numonyx R.K. 2009/06/08
				case (AMD_ID_GL128N_2 & 0xFF):
					debug ("Chip: S29GL128\n");
					info->flash_id += FLASH_S29GL128N;
					info->sector_count = 128;
					info->size = 0x01000000;
					for (i = 0; i < info->sector_count; i++) {
						info->start[i] = base;
						base += 0x20000;
					}
				break;	/* => 16 MB	*/
		  }
		}
		else														// add by Numonyx R.K. 2009/06/08
		{
			printf ("Mirror Bit flash: %04X\n", DevID2);	// changed by Numonyx R.K. 2009/06/08

			switch(DevID2)
			{											// changed by Numonyx R.K. 2009/06/08
				case (AMD_ID_GL128N_2 & 0xFF):
					debug ("Chip: S29GL128\n");
					info->flash_id += FLASH_S29GL128N;
					info->sector_count = 128;
					info->size = 0x01000000;
					for (i = 0; i < info->sector_count; i++) {
						info->start[i] = base;
						base += 0x20000;
					}
				break;	/* => 16 MB	*/

				case (AMD_ID_GL256N_2 & 0xFF):
					debug ("Chip: S29GL256\n");
					info->flash_id += FLASH_S29GL256N;
					info->sector_count = 256;
					info->size = 0x02000000;
					for (i = 0; i < info->sector_count; i++) {
						info->start[i] = base;
						base += 0x20000;
					}
				break;	/* => 32 MB	*/

				case (AMD_ID_GL512N_2 & 0xFF):
					debug ("Chip: S29GL512\n");
					info->flash_id += FLASH_S29GL512N;
					info->sector_count = 512;
					info->size = 0x04000000;
					for (i = 0; i < info->sector_count; i++) {
						info->start[i] = base;
						base += 0x20000;
					}
				break;	/* => 64 MB	*/

				case (AMD_ID_GL1GN_2 & 0xFF):
					debug ("Chip: S29GL1G\n");
					info->flash_id += FLASH_S29GL1G;
					info->sector_count = 1024;
					info->size = 0x08000000;
					for (i = 0; i < info->sector_count; i++) {
						info->start[i] = base;
						base += 0x20000;
					}
				break;	/* => 128 MB	*/

				default:
					printf ("Chip: *** unknown ***\n");
					info->flash_id = FLASH_UNKNOWN;
					info->sector_count = 0;
					info->size = 0;
				break;
			}
		}
		break;
	case (EON_ID_LV160AB & 0xFF):
		debug ("Chip: EN29LV160AB\n");
		info->sector_count = 35;
		info->size = PHYS_FLASH_SIZE;

		info->start[0] = base;
		base += CONFIG_ENV_SECT_SIZE/4;
		info->start[1] = base;
		base += CONFIG_ENV_SECT_SIZE/8;
		info->start[2] = base;
		base += CONFIG_ENV_SECT_SIZE/8;
		info->start[3] = base;
		base += CONFIG_ENV_SECT_SIZE/2;

		for (i = 4; i < info->sector_count; i++) {
			info->start[i] = base;
			base += CONFIG_ENV_SECT_SIZE;
		}
	break;
	default:
		//printf ("Unknown Device ID: %04X\n", value);
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->size = 0;
	break;
	}

out:
	/* Put FLASH back in read mode */
	flash_reset(info);

	return (info->size);
}

/*-----------------------------------------------------------------------
 */

int	flash_erase (flash_info_t *info, int s_first, int s_last)
{
#ifdef eight_bus_width
	vu_char *addr = (vu_short *)(info->start[0]);
#else
	vu_short *addr = (vu_short *)(info->start[0]);
#endif
	int flag, prot, sect;
	ulong now, last;

	debug ("flash_erase: first: %d last: %d\n", s_first, s_last);

	if ((s_first < 0) || (s_first > s_last)) {
		if (info->flash_id == FLASH_UNKNOWN) {
			printf ("- missing\n");
		} else {
			printf ("- no sectors to erase\n");
		}
		return 1;
	}

	if ((info->flash_id == FLASH_UNKNOWN) ||
	    (info->flash_id > FLASH_MAN_ATM)) {
		printf ("Can't erase unknown flash type %08lx - aborted\n",
			info->flash_id);
		return 1;
	}

	prot = 0;
	for (sect=s_first; sect<=s_last; ++sect) {
		if (info->protect[sect]) {
			prot++;
		}
	}

	if (prot) {
		printf ("- Warning: %d protected sectors will not be erased!\n",
			prot);
	} else {
		printf ("\n");
	}

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts();

	/*
	 * Start erase on unprotected sectors.
	 * Since the flash can erase multiple sectors with one command
	 * we take advantage of that by doing the erase in chunks of
	 * 3 sectors.
	 */
	for (sect = s_first; sect <= s_last; sect++)
	{
		if (info->protect[sect] == 0) {	/* not protected */
			addr = (vu_short *)(info->start[sect]);
	#ifdef eight_bus_width
			addr[FLASH_CYCLE1] = 0x00AA;
			addr[FLASH_CYCLE2] = 0x0055;
			addr[FLASH_CYCLE1] = 0x0080;
			addr[FLASH_CYCLE1] = 0x00AA;
			addr[FLASH_CYCLE2] = 0x0055;
	#else
			addr[FLASH_HW_CYCLE1] = 0x00AA;
			addr[FLASH_HW_CYCLE2] = 0x0055;
			addr[FLASH_HW_CYCLE1] = 0x0080;
			addr[FLASH_HW_CYCLE1] = 0x00AA;
			addr[FLASH_HW_CYCLE2] = 0x0055;
	#endif
			addr[0] = 0x0030;

			/* wait at least 80us - let's wait 1 ms */
			udelay (1000);

			reset_timer_masked ();
			last  = 0;
#if 1
			while ((addr[0] & 0x0080) != 0x0080) {
				if ((now = get_timer_masked ()) > CFG_FLASH_ERASE_TOUT*20) {
					//printf ("Timeout\n");
					//return 1;
					putc ('o');
				}
				/* show that we're waiting */
				if ((now - last) > 1000) {	/* every second */
					putc ('.');
					last = now;
				}
			}
#else
			udelay (1000);
#endif

			addr[0] = 0x00F0;	/* reset bank */
		}//for if protected
	}
	/* re-enable interrupts if necessary */
	if (flag)
		enable_interrupts();

	/* reset to read mode */
	addr = (vu_short *)info->start[0];
	addr[0] = 0x00F0;	/* reset bank */

	printf (" done\n");
	return 0;
}

/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
int write_buff (flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
	ulong wp, data;
	int rc;
	int buffer_count;
	ulong now, last;

//printf("src=%x,addr=%x,cnt=%x\n",src,addr,cnt);

	if (addr & 1) {
		//printf ("unaligned destination not supported\n");
		addr++;
		//return ERR_ALIGN;
	};

	if ((int) src & 1) {
		//printf ("unaligned source not supported\n");
		//return ERR_ALIGN;
	};

	// Insert the buffer programming mode in 256 Words only.
	// 512 Words only can be done when block alligned.
	// Added by Numonyx R.K. 2009/06/05
	// --------------------------------------------------------


	//printf("info->flash_id = %x\n",info->flash_id & FLASH_VENDMASK);
	if((info->flash_id & FLASH_VENDMASK) == FLASH_MAN_NUMONYX)
	{
		buffer_count = cnt / 256;
		printf("buffer_count = %d\n", buffer_count);
		while (buffer_count--)
		{
			write_buffer_nmx(info, (vu_char *)addr, src);
			//printf("Write Numonyx Buffer addr = %x\n",(u32)addr);
			//printf("Write Numonyx Buffer *src = %x\n",(u32 *)src);
			addr +=256;
			src +=256;
			putc ('.');
		}
		cnt = cnt % 256;

	}
	// -----------------------------------------------------------

	wp = addr;
	reset_timer_masked ();
	last  = 0;

	while (cnt >= 2) {

		if ((int) src & 1)
			data = (*((vu_char *)(src+1)) << 8) | (*((vu_char *)src));//*((vu_short *)src);
		else
			data = *((vu_short *)src);
		if ((rc = write_word_amd(info, (vu_short *)wp, data)) != 0) {
printf ("write_buff 1: write_word_amd() rc=%d\n", rc);
			return (rc);
		}
		src += 2;
		wp += 2;
		cnt -= 2;

		/* show that we're waiting */
		now = get_timer_masked ();
		if ((now - last) > 1000) {	/* every second */
				putc ('.');
				last = now;
		}
	}

	if (cnt == 0) {
		return (ERR_OK);
	}

	if (cnt == 1) {
		data = (*((volatile u8 *) src)) | (*((volatile u8 *) (wp + 1)) << 8);
		if ((rc = write_word_amd(info, (vu_short *)wp, data)) != 0) {
printf ("write_buff 2: write_word_amd() rc=%d\n", rc);
			return (rc);
		}
		src += 1;
		wp += 1;
		cnt -= 1;
	}

	return ERR_OK;
}

/*-----------------------------------------------------------------------
 * Write a word to Flash for AMD FLASH
 * A word is 16 or 32 bits, whichever the bus width of the flash bank
 * (not an individual chip) is.
 *
 * returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */

static int write_word_amd (flash_info_t *info, vu_short *dest, vu_short data)
{
	int flag;
#ifdef eight_bus_width
  vu_char *dd, *dest_tmp;
  uchar data_byte[2],i;
#else
	vu_short *base;		/* first address in flash bank	*/
#endif
	/* Check if Flash is (sufficiently) erased */
	//printf("<%x>",*dest);
	//putc ('r');
	if ((*dest & data) != data) {

	//*(volatile ulong *)(CONFIG_GPIO_BASE + 0x0) &= ~(1<<0);
	//*(volatile ulong *)(CONFIG_GPIO_BASE + 0x8) |= (1<<0);
	//printf("data=%x,addr=%x",data,dest);
		return (2);
	}

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts();

#ifndef eight_bus_width
	base = (vu_short *)(info->start[0]);
	*dest = data;		/* start programming the data	*/

	base[FLASH_HW_CYCLE1] = 0xAA;	/* unlock */
	base[FLASH_HW_CYCLE2] = 0x55;	/* unlock */
	base[FLASH_HW_CYCLE1] = 0xA0;	/* selects program mode */
#else
	dest_tmp = dest;
	dd = (vu_short *)(info->start[0]);

	data_byte[0] = data & 0x00ff;
	data_byte[1] = data >> 8;

	for(i=0;i<2;i++,dest_tmp++)
  {
		dd[FLASH_CYCLE1] = 0xAA;	/* unlock */
		dd[FLASH_CYCLE2] = 0x55;	/* unlock */
		dd[FLASH_CYCLE1] = 0xA0;	/* selects program mode */
		//putc ('w');

		*dest_tmp = data_byte[i];		/* start programming the data	*/
#endif
	//printf("base=%x,dest=%x,data=%x\n",base,dest,data);

	/* re-enable interrupts if necessary */
	if (flag)
		enable_interrupts();

	reset_timer_masked ();

#if 1
		/* data polling for D7 */
#ifndef eight_bus_width
		while ((*dest & 0x80) != (data & 0x80)) {
#else
		while ((*dest_tmp & 0x80) != (data_byte[i] & 0x80)) {
#endif
			if (get_timer_masked () > CFG_FLASH_ERASE_TOUT*20) {//CFG_FLASH_WRITE_TOUT) {
				//*dest = 0xF0;	/* reset bank */
				//return (1);
				putc ('o');
			}
		}
#else
	*(volatile ulong *)(CONFIG_GPIO_BASE + 0x0) &= ~(1<<0);
		udelay (500);
	*(volatile ulong *)(CONFIG_GPIO_BASE + 0x0) |= (1<<0);
#endif
#ifdef eight_bus_width
	}
#endif
	return (0);
}

/*-----------------------------------------------------------------------
 * Write a stream in Numonyx buffer mode to the flash.
 * This routin support 16-bit flash mode. It uses 256 word mode only
 * as we not check on block boundries so 256 words is the max. we can
 * write in once. It is a little slower but doing the job.
 *
 * returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 *-----------------------------------------------------------------------*/
static int write_buffer_nmx (flash_info_t *info, vu_char *dest, uchar *src)
{
    ulong start;
    int flag, i;
    int res = 0;	/* result, assume success	*/
    vu_char *base;		/* first address in flash bank	*/
		vu_char *data;
		vu_char *start_dest=dest;
    base = (vu_char *)(info->start[0]);

    /* Disable interrupts which might cause a timeout here */
    flag = disable_interrupts();

    base[FLASH_CYCLE1] = 0x00AA;	/* unlock */
    base[FLASH_CYCLE2] = 0x0055;	/* unlock */
    *dest        = 0x0025;   /* selects program mode */
		*dest        = 0x00FF;   /* N+1 in the buffer */


	data = src;

	for (i = 0; i < 256; i++)
	{
		*dest++ = *data;
		data++;
	}

	*start_dest		  = 0x0029; /* write buffer confirm */

    /* re-enable interrupts if necessary */
    if (flag)
	enable_interrupts();

    start = get_timer (0);

    /* data polling for DQ7 only not checking DQ5 and DQ1 like described in the datasheet */
    while (res == 0 && (*dest & 0x0080) != (*data & 0x0080))
	{
		if (get_timer(start) > CFG_FLASH_WRITE_TOUT)
		{
			*dest = 0x00F0;	/* reset bank */
			res = 1;
		}
    }

    return (res);
}
/*-----------------------------------------------------------------------*/