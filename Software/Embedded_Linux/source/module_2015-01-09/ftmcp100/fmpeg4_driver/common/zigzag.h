#ifndef _ZIGZAG_H_
	#define _ZIGZAG_H_

	#ifdef ZIGZAG_GLOBAL
		#define ZIGZAG_EXT
	#else
		#define ZIGZAG_EXT extern
	#endif

	/* zig_zag_scan */
	ZIGZAG_EXT uint8_t scan_tables[64];
#endif							/* _ZIGZAG_H_ */
