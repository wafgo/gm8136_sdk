#ifndef _FT3DNR_DBG_H_
#define _FT3DNR_DBG_H_

#define dn_err(fmt, arg...)       do {printk(KERN_ERR     "[3DNR_ERR]: "    fmt, ## arg);} while (0)  /* error conditions                 */
#define dn_warn(fmt, arg...)      do {printk(KERN_WARNING "[3DNR_WARN]: "   fmt, ## arg);} while (0)  /* warning conditions               */
#define dn_notice(fmt, arg...)    do {printk(KERN_NOTICE  "[3DNR_NOTICE]: " fmt, ## arg);} while (0)  /* normal but significant condition */
#define dn_info(fmt, arg...)      do {printk(KERN_INFO    "[3DNR_INFO]: "   fmt, ## arg);} while (0)  /* informational                    */
#define dn_dbg(fmt, arg...)       do {printk(KERN_DEBUG   "[3DNR_DBG]: "    fmt, ## arg);} while (0)  /* debug-level messages             */
#endif