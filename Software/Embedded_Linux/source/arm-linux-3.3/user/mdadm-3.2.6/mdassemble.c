/*
 * mdassemble - assemble Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2009 Neil Brown <neilb@suse.de>
 * Copyright (C) 2003 Luca Berra <bluca@vodka.it>
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
 *    Email: <neilb@suse.de>
 */

#include "mdadm.h"
#include "md_p.h"

#ifndef MDASSEMBLE_AUTO
/* from mdopen.c */
int open_mddev(char *dev, int report_errors/*unused*/)
{
	int mdfd = open(dev, O_RDWR);
	if (mdfd < 0)
		fprintf(stderr, Name ": error opening %s: %s\n",
			dev, strerror(errno));
	else if (md_get_version(mdfd) <= 0) {
		fprintf(stderr, Name ": %s does not appear to be an md device\n",
			dev);
		close(mdfd);
		mdfd = -1;
	}
	return mdfd;
}
int create_mddev(char *dev, char *name, int autof/*unused*/, int trustworthy,
		 char *chosen)
{
	return open_mddev(dev, 0);
}
#endif
int map_update(struct map_ent **mpp, int devnum, char *metadata,
	       int *uuid, char *path)
{
	return 0;
}
struct map_ent *map_by_name(struct map_ent **mpp, char *name)
{
	return NULL;
}

int rv;
int mdfd = -1;
int runstop = 0;
int readonly = 0;
int verbose = 0;
int force = 0;

int main(int argc, char *argv[]) {
	struct mddev_ident *array_list =  conf_get_ident(NULL);
	if (!array_list) {
		fprintf(stderr, Name ": No arrays found in config file\n");
		rv = 1;
	} else
		for (; array_list; array_list = array_list->next) {
			mdu_array_info_t array;
			if (strcasecmp(array_list->devname, "<ignore>") == 0)
				continue;
			mdfd = open_mddev(array_list->devname, 0);
			if (mdfd >= 0 && ioctl(mdfd, GET_ARRAY_INFO, &array) == 0) {
				rv |= Manage_ro(array_list->devname, mdfd, -1); /* make it readwrite */
				continue;
			}
			if (mdfd >= 0)
				close(mdfd);
			rv |= Assemble(array_list->st, array_list->devname,
				       array_list, NULL, NULL, 0,
				       readonly, runstop, NULL, NULL, 0,
				       verbose, force, 1);
		}
	return rv;
}
