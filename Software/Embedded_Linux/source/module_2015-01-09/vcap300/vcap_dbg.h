#ifndef _VCAP_DBG_H_
#define _VCAP_DBG_H_

#define vcap_err(fmt, arg...)       do {printk(KERN_ERR     "[VCAP_ERR]: "    fmt, ## arg);} while (0)  /* error conditions                 */
#define vcap_dbg(fmt, arg...)       do {printk(KERN_DEBUG   "[VCAP_DBG]: "    fmt, ## arg);} while (0)  /* debug-level messages             */
#define vcap_info(fmt, arg...)      do {printk(KERN_INFO    "[VCAP_INFO]: "   fmt, ## arg);} while (0)  /* informational                    */
#define vcap_warn(fmt, arg...)      do {printk(KERN_WARNING "[VCAP_WARN]: "   fmt, ## arg);} while (0)  /* warning conditions               */
#define vcap_notice(fmt, arg...)    do {printk(KERN_NOTICE  "[VCAP_NOTICE]: " fmt, ## arg);} while (0)  /* normal but significant condition */

#define vcap_err_limit(fmt, arg...) do {if(printk_ratelimit()) printk(KERN_ERR "[VCAP_ERR]: " fmt, ## arg);} while (0)  /* error conditions with printk rate limit control */

#endif  /* _VCAP_DBG_H_ */
