#ifndef _PLATFORM_H_
#define _PLATFORM_H_

extern int pf_codec_exit(void);
extern int pf_codec_on(void);
extern void pf_codec_off(void);

#ifdef ENABLE_SWITCH_CLOCK
    extern int pf_clock_switch_on(void);
    extern void pf_clock_switch_off(void);
#endif

#endif
