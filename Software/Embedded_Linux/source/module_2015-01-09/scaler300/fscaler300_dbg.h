#ifndef _FSCALER300_DBG_H_
#define _FSCALER300_DBG_H_

#define scl_err(fmt, arg...)       do {printk(KERN_ERR     "[SCL_ERR]: "    fmt, ## arg);} while (0)  /* error conditions                 */
#define scl_warn(fmt, arg...)      do {printk(KERN_WARNING "[SCL_WARN]: "   fmt, ## arg);} while (0)  /* warning conditions               */
#define scl_notice(fmt, arg...)    do {printk(KERN_NOTICE  "[SCL_NOTICE]: " fmt, ## arg);} while (0)  /* normal but significant condition */
#define scl_info(fmt, arg...)      do {printk(KERN_INFO    "[SCL_INFO]: "   fmt, ## arg);} while (0)  /* informational                    */
#define scl_dbg(fmt, arg...)       do {printk(KERN_DEBUG   "[SCL_DBG]: "    fmt, ## arg);} while (0)  /* debug-level messages             */
#endif