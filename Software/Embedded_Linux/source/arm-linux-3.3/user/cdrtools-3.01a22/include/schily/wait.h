/* @(#)wait.h	1.19 11/07/19 Copyright 1995-2011 J. Schilling */
/*
 *	Definitions to deal with various kinds of wait flavour
 *
 *	Copyright (c) 1995-2011 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_WAIT_H
#define	_SCHILY_WAIT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * Needed for SysVr4
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#if	defined(HAVE_WAIT_H)
#	ifndef	_INCL_WAIT_H
#	include <wait.h>
#	define	_INCL_WAIT_H
#	endif
#else
/*
 * K&R Compiler doesn't like #elif
 */
#	if	defined(HAVE_SYS_WAIT_H)	/* POSIX.1 compl. sys/wait.h */
#	undef	HAVE_UNION_WAIT			/* POSIX.1 doesn't use U_W   */
#		ifndef	_INCL_SYS_WAIT_H
#		include <sys/wait.h>
#		define	_INCL_SYS_WAIT_H
#		endif
#	else
#	if	defined(HAVE_UNION_WAIT)	/* Pure BSD U_W / sys/wait.h */
#		ifndef	_INCL_SYS_WAIT_H
#		include <sys/wait.h>
#		define	_INCL_SYS_WAIT_H
#		endif
#	endif
#	endif
#endif
#ifdef	JOS
#	ifndef	_INCL_SYS_EXIT_H
#	include <sys/exit.h>
#	define	_INCL_SYS_EXIT_H
#	endif
#endif
#if defined(__EMX__) || defined(__DJGPP__)
#	ifndef	_INCL_PROCESS_H
#	include <process.h>
#	define	_INCL_PROCESS_H
#	endif
#endif


#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	WCOREFLG
#define	WCOREFLG	0x80
#define	NO_WCOREFLG
#endif

#ifndef	WSTOPFLG
#define	WSTOPFLG	0x7F
#define	NO_WSTOPFLG
#endif

#ifndef	WCONTFLG
#define	WCONTFLG	0xFFFF
#define	NO_WCONTFLG
#endif

/*
 * waitid() option flags:
 */
#ifndef	WCONTINUED
#define	WCONTINUED	0
#define	NO_WCONTINUED
#endif
#ifndef	WEXITED
#define	WEXITED		0
#define	NO_WEXITED
#endif
#ifndef	WNOHANG
#define	WNOHANG		0
#define	NO_WNOHANG
#endif
#ifndef	WNOWAIT
#define	WNOWAIT		0
#define	NO_WNOWAIT
#endif
#ifndef	WSTOPPED
#define	WSTOPPED	0
#define	NO_WSTOPPED
#endif
#ifndef	WTRAPPED
#define	WTRAPPED	0
#define	NO_WTRAPPED
#endif

#if defined(HAVE_UNION_WAIT) && defined(USE_UNION_WAIT)
#	define WAIT_T union wait
#	define	_W_U(w)	((union wait *)&(w))
#	ifndef WSTOPPED
#		define	WSTOPPED	0x7F
#	endif
#	ifndef WTERMSIG
#		define WTERMSIG(status)		(_W_U(status)->w_termsig)
#	endif
#	ifndef WCOREDUMP
#		define WCOREDUMP(status)	(_W_U(status)->w_coredump)
#	endif
#	ifndef WEXITSTATUS
#		define WEXITSTATUS(status)	(_W_U(status)->w_retcode)
#	endif
#	ifndef WSTOPSIG
#		define WSTOPSIG(status)		(_W_U(status)->w_stopsig)
#	endif
#	ifndef WIFCONTINUED
#		define	WIFCONTINUED(status)	(0)
#	endif
#	ifndef WIFSTOPPED
#		define WIFSTOPPED(status)	(_W_U(status)->w_stopval == \
								WSTOPPED)
#	endif
#	ifndef WIFSIGNALED
#		define WIFSIGNALED(status) 	(_W_U(status)->w_stopval != \
						WSTOPPED && \
						_W_U(status)->w_termsig != 0)
#	endif
#	ifndef WIFEXITED
#		define WIFEXITED(status)	(_W_U(status)->w_stopval != \
						WSTOPPED && \
						_W_U(status)->w_termsig == 0)
#	endif
#else
#	define WAIT_T int
#	define	_W_I(w)	(*(int *)&(w))
#	ifndef WTERMSIG
#		define WTERMSIG(status)		(_W_I(status) & 0x7F)
#	endif
#	ifndef WCOREDUMP
#		define WCOREDUMP(status)	(_W_I(status) & 0x80)
#	endif
#	ifndef WEXITSTATUS
#		define WEXITSTATUS(status)	((_W_I(status) >> 8) & 0xFF)
#	endif
#	ifndef WSTOPSIG
#		define WSTOPSIG(status)		((_W_I(status) >> 8) & 0xFF)
#	endif
/*
 * WIFSTOPPED and WIFSIGNALED match the definitions on older UNIX versions
 * e.g. SunOS-4.x or HP-UX
 */
#	ifndef WIFCONTINUED
#	ifdef	NO_WCONTINUED
#		define	WIFCONTINUED(status)	(0)
#	else
#		define	WIFCONTINUED(status)	((_W_I(status) & 0xFFFF) == \
								WCONTFLG)
#	endif
#	endif
#	ifndef WIFSTOPPED
#		define	WIFSTOPPED(status)	((_W_I(status) & 0xFF) == 0x7F)
#	endif
#	ifndef WIFSIGNALED
#		define	WIFSIGNALED(status)	((_W_I(status) & 0xFF) != 0x7F && \
						WTERMSIG(status) != 0)
#	endif
#	ifndef WIFEXITED
#		define	WIFEXITED(status)	((_W_I(status) & 0xFF) == 0)
#	endif
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_WAIT_H */
