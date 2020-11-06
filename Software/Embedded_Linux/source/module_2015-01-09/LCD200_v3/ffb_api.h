#ifndef __FFB_API_H__
#define __FFB_API_H__

#include <linux/fb.h>
#include "lcd_fb.h"

#define FFB_API_VER(m,s) ((m)<<16|(s))
#define FFB_API_MAIN(v)  (((v)>>16)&0xffff)
#define FFB_API_SUB(v)   ((v)&0xffff)

#endif /* __FFB_API_H__ */
