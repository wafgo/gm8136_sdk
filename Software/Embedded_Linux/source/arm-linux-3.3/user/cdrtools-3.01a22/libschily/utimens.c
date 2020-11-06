/* @(#)utimens.c	1.2 13/10/30 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of utimens(const char *name,
 *					const struct timespec times[2])
 *
 *	Copyright (c) 2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/utime.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_UTIMENS

EXPORT int
utimens(name, times)
	const char		*name;
	const struct timespec	times[2];
{
#ifdef	HAVE_UTIMENSAT
	return (utimensat(AT_FDCWD, name, times, 0));
#else
#ifdef	HAVE_UTIMES
	struct timeval tv[2];

	if (times == NULL)
		return (utimes(name, NULL));
	tv[0].tv_sec  = times[0].tv_sec;
	tv[0].tv_usec = times[0].tv_nsec/1000;
	tv[1].tv_sec  = times[1].tv_sec;
	tv[1].tv_usec = times[1].tv_nsec/1000;
	return (utimes(name, tv));

#else
#ifdef	HAVE_UTIME
	struct utimbuf	ut;

	if (times == NULL)
		return (utime(name, NULL));
	ut.actime = times[0].tv_sec;
	ut.modtime = times[1].tv_sec;

	return (utime(name, &ut));
#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
#endif
#endif
}

#endif	/* HAVE_UTIMENS */
