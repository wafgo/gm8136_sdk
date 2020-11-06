/* jconfig.vc --- jconfig.h for Microsoft Visual C++ on Windows 95 or NT. */
/* see jconfig.doc for explanations */

#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H



/*
// Define "boolean" as unsigned char, not int, per Windows custom 
#ifndef __RPCNDR_H__		// don't conflict if rpcndr.h already read 
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		// prevent jmorecfg.h from redefining it 
*/


#ifdef JPEG_CJPEG_DJPEG

//#define BMP_SUPPORTED		// BMP image file format
//#define GIF_SUPPORTED		// GIF image file format
//#define PPM_SUPPORTED		// PBMPLUS PPM/PGM image file format 
//#undef RLE_SUPPORTED		// Utah RLE image file format 
//#define TARGA_SUPPORTED		// Targa image file format 

#define TWO_FILE_COMMANDLINE	/* optional */

// comment it for ARM's Compiler
//#define USE_SETMODE		/* Microsoft has setmode() */

//#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
//#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */

#define NO_MKTEMP		// don't use mktemp() function under ARM processor
