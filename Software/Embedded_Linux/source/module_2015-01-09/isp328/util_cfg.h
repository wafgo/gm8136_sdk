/**
 * @file util_cfg.h
 * CFG file utilities header file
 *
 * Copyright (C) 2014 GM Corp. (http://www.grain-media.com)
 *
 */

#ifndef __UTIL_CFG_H__
#define __UTIL_CFG_H__

/*
 * Set line length in configuration files
 */
#define FALSE           0
#define TRUE            1
#define LINE_LEN        2048
#define RESET           -9999
#define CHAR_0          '0'
#define CHAR_9          '9'
#define CHAR_SPACE      ' '
#define CHAR_NEWLINE    '\n'
#define CHAR_CR         '\n'
#define CHAR_RET        '\r'
#define CHAR_NULL       '\0'
#define CHAR_TAB        '\t'
#define CHAR_EQUAL      '='
#define CHAR_LSQR       '['
#define CHAR_RSQR       ']'
#define CHAR_COMMENT1   '#'
#define CHAR_COMMENT2   '/'
#define STR_COMMENT2    "//"

/*
 * Define return error code value
 */
#define ERR_NONE        0   /* read configuration file successfully */
#define ERR_NOFILE      2   /* not find or open configuration file */
#define ERR_READFILE    3   /* error occur in reading configuration file */
#define ERR_FORMAT      4   /* invalid format in configuration file */
#define ERR_NOTHING     5   /* not find section or key name in configuration file */

/*
 * Utilitiy macros
 */
#define is_digit(c)     (((c) >= CHAR_0) && ((c) <= CHAR_9))
#define is_not_digit(c) (((c) < CHAR_0) || ((c) > CHAR_9))
#define is_space(c)     ((c == CHAR_SPACE))

/*
 * File structure and operations
 */
typedef struct file_s {
    struct file *filp;
    loff_t offset;
    mm_segment_t fs;
    char line_buf[LINE_LEN];
} file_t;

#define StrToLong       simple_strtol
#define StrToULong      simple_strtoul
#define SEE             printk

#define alloc_mem(s)    kmalloc((s),GFP_KERNEL)

extern file_t *gmcfg_open( const char*, const int ) ;
extern void gmcfg_close(file_t *f);

/*
 * Read the value of key name in string form
 */
extern int gmcfg_getfieldstr(const char *section,   /* points to section name */
                             const char *keyname,   /* points to key name */
                             char *buf,             /* points to destination buffer address */
                             int size,              /* destination buffer size */
                             file_t *cfile);        /* points to configuration file */
/*
 * Convert string to s32
 */
#define gmcfg_strtol(str)   simple_strtol(str, NULL, 0)

/*
 * Convert string to u32
 */
#define gmcfg_strtoul(str)  simple_strtoul(str, NULL, 0)

/*
 * Convert string to u32 array
 */
extern int gmcfg_strtotab_u32(char* str, u32* tab, int count);

/*
 * Convert string to s32 array
 */
extern int gmcfg_strtotab_s32(char* str, s32* tab, int count);

/*
 * Convert string to u16 array
 */
extern int gmcfg_strtotab_u16(char* str, u16* tab, int count);

/*
 * Convert string to s16 array
 */
extern int gmcfg_strtotab_s16(char* str, s16* tab, int count);

/*
 * Convert string to u8 array
 */
extern int gmcfg_strtotab_u8(char* str, u8* tab, int count);

/*
 * Convert string to s8 array
 */
extern int gmcfg_strtotab_s8(char* str, s8* tab, int count);

extern file_t *isp_fopen(const char* name, const int flag);
extern ssize_t isp_fread(file_t *f, char *buffer, size_t len);
extern ssize_t isp_fwrite(file_t *f, const char *buffer, size_t len);
extern void isp_fclose(file_t *f);

#endif /* __UTIL_CFG_H__ */
