/*
 * open-cd-excl.c --- This program tries to open a block device
 * by various exclusive and non-exclusive gestures in order to explore
 * their impact on running CD/DVD recordings.
 *
 * Copyright 2007, by Theodore Ts'o.
 *
 * Detail modifications 2007, by Thomas Schmitt.
 * 
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#define _GNU_SOURCE /* for O_LARGEFILE *//*ts A70417: or _LARGEFILE64_SOURCE */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>

const char *progname;

static void usage(void)
{
	fprintf(stderr, "Usage: %s [-feirw] device\n", progname);
	exit(1);
}

/* ts A70417: added parameter do_rdwr */
static void init_flock(struct flock *fl, int do_rdwr)
{
	memset(fl, 0, sizeof(struct flock));
	if (do_rdwr)
		fl->l_type = F_WRLCK;
	else
		fl->l_type = F_RDLCK;
	fl->l_whence = SEEK_SET;
	fl->l_start = 0;
	fl->l_len = 0;
}

int main(int argc, char **argv)
{
	struct flock	fl;
	char	*device_name;
	int	fd, c, f_opt = 0, do_rdwr = 0, end_immediately = 0;
	int	flags = O_NONBLOCK|O_LARGEFILE;
	
	progname = argv[0];

	/* ts A70417: added -w , -r , -i */
	while ((c = getopt (argc, argv, "feirw")) != EOF) {
		switch (c) {
		case 'e':
			flags |= O_EXCL;
			break;
		case 'f':
			f_opt++;
			break;
		case 'i':
			end_immediately = 1;
			break;
		case 'r':
			do_rdwr = 0;
			break;
		case 'w':
			do_rdwr = 1;
			break;
		case '?':
			usage();
			exit(1);
		}
	}

	if (optind == argc)
		usage();
	device_name = argv[optind++];

	/* ts A70417 : made read-write adjustable independently of f_opt */
	if (do_rdwr) {
		flags |= O_RDWR;
		printf("Using O_RDWR\n");
	} else {
		flags |= O_RDONLY;
		printf("Using O_RDONLY\n");
	}

	if (flags & O_EXCL)
		printf("Trying to open %s with O_EXCL ...\n", device_name);
	fd = open(device_name, flags, 0);
	if (fd < 0) {
		perror("open");
		printf("failed\n");
		exit(1);
	}
	if (flags & O_EXCL)
		printf("succeeded\n");
		
	if (f_opt) {
		init_flock(&fl, do_rdwr);
		if (fcntl(fd, F_GETLK, &fl) < 0) {
			perror("fcntl: F_GETLK: ");
			exit(1);
		}
		printf("fcntl lock apparently %sLOCKED\n", 
		       (fl.l_type == F_UNLCK) ? "NOT " : "");

		init_flock(&fl, do_rdwr);
		printf("Trying to grab fcntl lock...\n");
		if (fcntl(fd, F_SETLK, &fl) < 0) {
			perror("fcntl: F_SETLK: ");
			printf("failed\n");
			exit(1);
		}
		printf("succeeded\n");
	}

	/* ts A70417: added end_immediately */
	printf("Holding %s open.\n", device_name);
	usleep(100000);
	if (end_immediately)
		exit(0);
	printf("Press ^C to exit.\n");
	while (1) {
		sleep(300);
	} 
	/* NOTREACHED */
	return 0;
}
