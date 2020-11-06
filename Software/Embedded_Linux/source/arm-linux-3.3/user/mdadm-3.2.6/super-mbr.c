/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2010 Neil Brown <neilb@suse.de>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Author: Neil Brown
 *    Email: <neil@brown.name>
 *
 */

/*
 * 'mbr' is a pseudo metadata type for devices which have a
 * partition table in the Master Boot Record (mbr) also known
 * as a dos partition table.
 *
 * Obviously arrays cannot be created or assembled for this type.
 * It is used to allow a new bare device to have an partition table
 * added so the member partitions can then be included in other
 * arrays as relevant.
 *
 * The meaning operations are:
 * examine_super, but not brief_examine_super or export_examine
 * load_super
 * store_super
 */

#include "mdadm.h"
#include "part.h"

static void free_mbr(struct supertype *st)
{
	free(st->sb);
	st->sb = NULL;
}

#ifndef MDASSEMBLE

static void examine_mbr(struct supertype *st, char *homehost)
{
	struct MBR *sb = st->sb;
	int i;

	printf("   MBR Magic : %04x\n", sb->magic);
	for (i = 0; i < MBR_PARTITIONS; i++)
		if (sb->parts[i].blocks_num)
			printf("Partition[%d] : %12lu sectors at %12lu (type %02x)\n",
			       i,
			       (unsigned long)__le32_to_cpu(sb->parts[i].blocks_num),
			       (unsigned long)__le32_to_cpu(sb->parts[i].first_sect_lba),
			       sb->parts[i].part_type);

}

#endif /*MDASSEMBLE */

static int load_super_mbr(struct supertype *st, int fd, char *devname)
{
	/* try to read an mbr
	 * Return
	 *  0 on success
	 *  1 cannot get record
	 *  2 record is meaningless
	 */
	struct MBR *super;

	free_mbr(st);

	if (posix_memalign((void**)&super, 512, 512) != 0) {
		fprintf(stderr, Name ": %s could not allocate superblock\n",
			__func__);
		return 1;
	}

	ioctl(fd, BLKFLSBUF, 0); /* make sure we read current data */

	lseek(fd, 0, 0);
	if (read(fd, super, sizeof(*super)) != sizeof(*super)) {
		if (devname)
			fprintf(stderr, Name ": Cannot read partition table on %s\n",
				devname);
		free(super);
		return 1;
	}
 
	if (super->magic != MBR_SIGNATURE_MAGIC) {
		if (devname)
			fprintf(stderr, Name ": No partition table found on %s\n",
				devname);
		free(super);
		return 1;
	}

	st->sb = super;

	if (st->ss == NULL) {
		st->ss = &mbr;
		st->minor_version = 0;
		st->max_devs = 1;
		st->info = NULL;
	}
	return 0;
}

static int store_mbr(struct supertype *st, int fd)
{
	struct MBR *old, *super;

	if (posix_memalign((void**)&old, 512, 512) != 0) {
		fprintf(stderr, Name ": %s could not allocate superblock\n",
			__func__);
		return 1;
	}

	ioctl(fd, BLKFLSBUF, 0); /* make sure we read current data */

	lseek(fd, 0, 0);
	if (read(fd, old, sizeof(*old)) != sizeof(*old)) {
		free(old);
		return 1;
	}

	super = st->sb;
	memcpy(super->pad, old->pad, sizeof(super->pad));
	free(old);
	lseek(fd, 0, 0);
	if (write(fd, super, sizeof(*super)) != sizeof(*super))
		return 4;
	fsync(fd);
	ioctl(fd, BLKRRPART, 0);
	return 0;
}

static void getinfo_mbr(struct supertype *st, struct mdinfo *info, char *map)
{
	struct MBR *sb = st->sb;
	int i;

	memset(&info->array, 0, sizeof(info->array));
	memset(&info->disk, 0, sizeof(info->disk));
	strcpy(info->text_version, "mbr");
	strcpy(info->name, "mbr");
	info->component_size = 0;

	for (i = 0; i < MBR_PARTITIONS ; i++)
		if (sb->parts[i].blocks_num) {
			unsigned long last = 
				(unsigned long)__le32_to_cpu(sb->parts[i].blocks_num)
				+ (unsigned long)__le32_to_cpu(sb->parts[i].first_sect_lba);
			if (last > info->component_size)
				info->component_size = last;
		}

}

static struct supertype *match_metadata_desc(char *arg)
{
	struct supertype *st;

	if (strcmp(arg, "mbr") != 0)
		return NULL;

	st = malloc(sizeof(*st));
	if (!st)
		return st;
	st->ss = &mbr;
	st->info = NULL;
	st->minor_version = 0;
	st->max_devs = 1;
	st->sb = NULL;
	return st;
}

#ifndef MDASSEMBLE
static int validate_geometry(struct supertype *st, int level,
			     int layout, int raiddisks,
			     int *chunk, unsigned long long size,
			     char *subdev, unsigned long long *freesize,
			     int verbose)
{
	fprintf(stderr, Name ": mbr metadata cannot be used this way\n");
	return 0;
}
#endif

struct superswitch mbr = {
#ifndef MDASSEMBLE
	.examine_super = examine_mbr,
	.validate_geometry = validate_geometry,
#endif
	.match_metadata_desc = match_metadata_desc,
	.load_super = load_super_mbr,
	.store_super = store_mbr,
	.getinfo_super = getinfo_mbr,
	.free_super = free_mbr,
	.name = "mbr",
};
