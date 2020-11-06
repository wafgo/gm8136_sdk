/*
 * (C) Copyright 2006
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
#include <command.h>
#include <malloc.h>
#include <image.h>
#include <asm/byteorder.h>
#include <usb.h>
#include <part.h>


#ifdef CONFIG_AUTO_UPDATE

#ifndef CONFIG_FOTG210
#error "must define CONFIG_FOTG210"
#endif

#ifndef CONFIG_USB_STORAGE
#error "must define CONFIG_USB_STORAGE"
#endif

#if !defined(CONFIG_CMD_FAT)
#error "must define CONFIG_CMD_FAT"
#endif

#undef AU_DEBUG

#undef debug
#ifdef	AU_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#else
#define debug(fmt,args...)
#endif	/* AU_DEBUG */

/* error number definiton */
#define ENONE   0   /* no error */
#define EGEN    1   /* general error */
#define ENOFILE 2   /* file not found */
#define EFLARGE 3   /* file too large */
#define ECMDERR 4   /* run command fail */
#define ENODEVC 5   /* device not found */

/* max number of partitions */
#define MAX_NUM_PARTS 10

static int au_usb_stor_curr_dev; /* current device */

/* max. number of files which could interest us */
#define AU_MAXFILES MAX_NUM_PARTS

/* where to load files into memory */
#define LOAD_ADDR 0x02000000

/* externals */
extern int fat_register_device(block_dev_desc_t *, int);
extern long file_fat_read(const char *, void *, unsigned long);
#ifdef CONFIG_CMD_NAND
#ifdef CONFIG_SPI_NAND_GM
extern SPI_NAND_SYSHDR *nand_sys_hdr;
#else
extern NAND_SYSHDR *nand_sys_hdr;
#endif
#endif
#ifdef CONFIG_CMD_SPI
extern struct spi020_flash spi_flash_info;
extern NOR_SYSHDR *spi_sys_hdr;
#endif

static inline int str2int(const char *p, int *num)
{
	char *endptr;

	*num = (int) simple_strtoull(p, &endptr, 10);
	return *p != '\0' && *endptr == '\0';
}

char *au_get_img_file(int idx)
{
    char *s = NULL, envstr[16];

    if (idx == -1)
        sprintf(envstr, "auimg%s", "aio");
    else
        sprintf(envstr, "auimg%d", idx);
    s = getenv(envstr);

    return s;
}

int au_fs_register_dev(void)
{
	block_dev_desc_t *stor_dev;
    int i;

	au_usb_stor_curr_dev = -1;
	/* start USB */
	if (usb_stop() < 0) {
		debug("usb_stop failed\n");
		return -1;
	}
	if (usb_init() < 0) {
		debug("usb_init failed\n");
		return -1;
	}
	/*
	 * check whether a storage device is attached (assume that it's
	 * a USB memory stick, since nothing else should be attached).
	 */
	au_usb_stor_curr_dev = usb_stor_scan(0);
	if (au_usb_stor_curr_dev == -1) {
		debug("No device found. Not initialized?\n");
		goto _exit;
	}

	/* check whether it has a partition table */
	stor_dev = get_dev("usb", 0);
	if (stor_dev == NULL) {
		debug("uknown device type\n");
		goto _exit;
	}

    for (i = 0; i <= 8; i++) {
    	if (fat_register_device(stor_dev, i) != 0) {
    		debug("Unable to use USB %d:%d for fatload\n",
    			au_usb_stor_curr_dev, i);
            if (i == 8)
                goto _exit;
    	} else {
    	    break;
    	}
    }

    return 0;

_exit:
	usb_stop();
    return -1;
}

int au_do_update(int idx, char *filename)
{
    char cmd_run[64];
    long filesz, partaddr, partsz;

#ifdef CONFIG_CMD_NAND
    partaddr = nand_sys_hdr->image[idx].addr;
    partsz = nand_sys_hdr->image[idx].size;
#endif
#ifdef CONFIG_CMD_SPI
    if (idx == -1) {
        partaddr = 0;
        partsz = spi_flash_info.size; /* whole flash */
    } else {
        partaddr = spi_sys_hdr->image[idx].addr;
        partsz = spi_sys_hdr->image[idx].size;
    }
#endif

    filesz = file_fat_read(filename, (void *)LOAD_ADDR, 0);
    if (filesz <= 0) {
        printf("file '%s' not found\n", filename);
        return -ENOFILE;
    }
    if (filesz > partsz) {
        printf("%s: file size 0x%lx exceed %s size 0x%lx\n",
            filename, filesz, (idx == -1) ? "flash" : "partition", partsz);
        return -EFLARGE;
    }

    /* erase partition */
#ifdef CONFIG_CMD_NAND
    sprintf(cmd_run, "nand erase.part part%d", idx);
    if (run_command(cmd_run, 0) != 0) {
        printf(">>>>>> erase partition %d failed\n", idx);
        return -ECMDERR;
    }
    /* update partition */
    sprintf(cmd_run, "nand write 0x%x 0x%lx 0x%lx", LOAD_ADDR, partaddr, filesz);
#endif

#ifdef CONFIG_CMD_SPI
    sprintf(cmd_run, "sf probe 0:0");
    if (run_command(cmd_run, 0) != 0) {
        printf(">>>>>> select spi flash failed\n");
        return -ECMDERR;
    }
    sprintf(cmd_run, "sf erase 0x%lx 0x%lx", partaddr, partsz);
    if (run_command(cmd_run, 0) != 0) {
        if (idx == -1)
            printf(">>>>>> erase whole flash failed\n");
        else
            printf(">>>>>> erase partition %d failed\n", idx);
        return -ECMDERR;
    }
    /* update partition */
    sprintf(cmd_run, "sf write 0x%x 0x%lx 0x%lx", LOAD_ADDR, partaddr, filesz);
#endif

    if (run_command(cmd_run, 0) != 0) {
        printf(">>>>>> update failed\n");
        return -ECMDERR;
    }

    return 0;
}

/*
 * this is called from board_init() after the hardware has been set up
 * and is usable. That seems like a good time to do this.
 * Right now the return value is ignored.
 */
int do_auto_update(void)
{
	int i, res = 0, old_ctrlc, sts;
	char *s, *filenm;

    /* Start to auto update if "autoupdate" is set to "yes" */
    if (((s = getenv("autoupdate")) != NULL) && (strcmp(s, "yes") == 0)) {
        printf("auto update process is starting\n\n");
    } else {
        debug("auto update is disabled\n\n");
        return -1;
    }

    if (au_fs_register_dev() < 0) {
        printf("usb device not found\n");
        res = -ENODEVC;
        goto xit;
    }

	/* make sure that we see CTRL-C and save the old state */
	old_ctrlc = disable_ctrlc(0);

	/* just loop thru all the possible files */
	for (i = -1; i < AU_MAXFILES; i++) {
#ifdef CONFIG_CMD_NAND
        /* NAND type flash not support whole flash update */
        if (i == -1)
            continue;
#endif
        filenm = au_get_img_file(i);
        if (filenm == NULL)
            continue;
        sts = au_do_update(i, filenm);
        if (sts < 0) {
            if (sts == -ENOFILE) {
                continue;
            } else {
                printf("\n>>>>>> %s: update fail\n\n", filenm);
                res = sts;
                break;
            }
        } else {
            printf("\n>>>>>> %s: update success\n\n", filenm);
            if (i == -1)
                break;
        }
	}

	/* restore the old state */
	disable_ctrlc(old_ctrlc);

xit:
    printf("\nauto update process is done\n");
    if (res == 0) {
        printf("please remove usb memory stick and then reboot system\n\n");
    } else {
        printf("but something wrong, please check log shown above\n\n");
    }

    if (res != -ENODEVC)
        while (1); /* halt system until reboot */

	return res;
}

int do_fwupd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int partno;
    char *filename;

    if ((argc != 2) && (argc != 3))
        return CMD_RET_USAGE;

    if (strcmp(argv[1], "aio") == 0) {
#ifdef CONFIG_CMD_NAND
        printf("Cannot update whole flash of NAND type, because of bad block issue.\n");
        return CMD_RET_FAILURE;
#endif
        partno = -1; /* indicate whole flash */
    } else {
        if (!str2int(argv[1], &partno)) {
            printf("'%s' is not a number\n", argv[1]);
            return CMD_RET_FAILURE;
        }
        if (partno >= MAX_NUM_PARTS) {
            printf("partition %d is not existing\n", partno);
            return CMD_RET_FAILURE;
        }
    }

    if (au_fs_register_dev() < 0) {
        printf("usb device not found\n");
        return CMD_RET_FAILURE;
    }

    if (argc == 3)
        filename = argv[2];
    else
        filename = au_get_img_file(partno);

    if (filename == NULL) {
        printf("image filename not specified\n");
        return CMD_RET_FAILURE;
    }

    if (au_do_update(partno, filename) < 0) {
        debug("firmware upgrade failed\n");
        return CMD_RET_FAILURE;
    }
    printf("\n>>>>>> %s: update success\n\n", filename);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	fwupd,	3,	1,	do_fwupd,
	"firmware upgrade from usb device for specified filename",
	"<part> [<filename>]\n"
	"    - Load binary file 'filename' from usb device\n"
	"      to update content of partition 'part' on flash.\n"
	"      If 'filename' is omitted, environment variable \"auimg?\"\n"
	"      is looking for, otherwise, command is aborted.\n"
	"      Parameter 'part' can be a number for partition\n"
	"      or a string \"aio\" for content of whole flash.\n"
	"      All numeric parameters are assumed to be decimal."
);
#endif /* CONFIG_AUTO_UPDATE */
