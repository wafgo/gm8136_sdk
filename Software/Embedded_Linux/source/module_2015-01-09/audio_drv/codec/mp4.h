#ifndef __MP4_H__
#define __MP4_H__


#include "decoder.h"

int FAADAPI AudioSpecificConfig(char *pBuffer,
                                   unsigned int buffer_size,
                                   mp4AudioSpecificConfig *mp4ASC);

int FAADAPI AudioSpecificConfig2(char *pBuffer,
                                    unsigned int buffer_size,
                                    mp4AudioSpecificConfig *mp4ASC,
                                    program_config *pce);

#endif
