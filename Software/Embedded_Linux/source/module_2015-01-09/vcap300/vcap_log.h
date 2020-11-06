#ifndef _VCAP_LOG_H_
#define _VCAP_LOG_H_

#include "log.h"        ///< header from module/include/videograph.include/log

#define VCAP_LOG_TAG    "VC"

#define vcap_log(fmt, arg...)       do {printm(VCAP_LOG_TAG, fmt, ## arg);} while (0)  /* log message to log module system */
#define vcap_damnit()               do {damnit(VCAP_LOG_TAG);} while (0)

#endif  /* _VCAP_LOG_H_ */