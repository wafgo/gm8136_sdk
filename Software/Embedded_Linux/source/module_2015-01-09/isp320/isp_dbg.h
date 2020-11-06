#ifndef __ISP_DBG_H__
#define __ISP_DBG_H__

#define isp_err(fmt, arg...)        do {printk(KERN_ERR     "[ISP_ERR]: "    fmt, ## arg);} while (0)  /* error conditions                 */
#define isp_dbg(fmt, arg...)        do {printk(KERN_DEBUG   "[ISP_DBG]: "    fmt, ## arg);} while (0)  /* debug-level messages             */
#define isp_info(fmt, arg...)       do {printk(KERN_INFO    "[ISP_INFO]: "   fmt, ## arg);} while (0)  /* informational                    */
#define isp_warn(fmt, arg...)       do {printk(KERN_WARNING "[ISP_WARN]: "   fmt, ## arg);} while (0)  /* warning conditions               */
#define isp_notice(fmt, arg...)     do {printk(KERN_NOTICE  "[ISP_NOTICE]: " fmt, ## arg);} while (0)  /* normal but significant condition */

#endif  /* __ISP_DBG_H__ */
