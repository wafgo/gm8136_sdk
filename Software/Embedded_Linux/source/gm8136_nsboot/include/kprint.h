#ifndef __KPRINT_H__
#define __KPRINT_H__

extern void KPrint(char const *, ...);

#ifndef	_VALIST
#define	_VALIST
typedef char *va_list;
#else
#include <stdarg.h>
#endif

/*
 * Storage alignment properties
 */
#define	_AUPBND		(sizeof(int) - 1)
#define _ADNBND		(sizeof(int) - 1)
#ifndef	_VALIST
/*
 * Variable argument list macro definitions
 */
#define _bnd(X, bnd)	(((sizeof (X)) + (bnd)) & (~(bnd)))
#define va_arg(ap, T)	(*(T *)(((ap) += (_bnd (T, _AUPBND))) - (_bnd (T,_ADNBND))))
#define va_end(ap)	(void) 0
#define va_start(ap, A) (void) ((ap) = (((char *) &(A)) + (_bnd (A,_AUPBND))))
#endif
#endif	/* __KPRINT_H__ */

