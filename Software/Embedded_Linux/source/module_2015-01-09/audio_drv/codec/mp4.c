#include "common.h"
#include "structs.h"
#include <stdlib.h>
#include "bits.h"
#include "mp4.h"
#include "syntax.h"

/* defines if an object type can be decoded by this library or not */
#if 0
// no check here by Shawn 2004.12.9
static char ObjectTypesTable[32] = 
{
    0, /*  0 NULL */
    0, /*  1 AAC Main */
    1, /*  2 AAC LC */
    0, /*  3 AAC SSR */
#ifdef LTP_DEC
    1, /*  4 AAC LTP */
#else
    0, /*  4 AAC LTP */
#endif
    0, /*  5 SBR */
    0, /*  6 AAC Scalable */
    0, /*  7 TwinVQ */
    0, /*  8 CELP */
    0, /*  9 HVXC */
    0, /* 10 Reserved */
    0, /* 11 Reserved */
    0, /* 12 TTSI */
    0, /* 13 Main synthetic */
    0, /* 14 Wavetable synthesis */
    0, /* 15 General MIDI */
    0, /* 16 Algorithmic Synthesis and Audio FX */

    /* MPEG-4 Version 2 */
    0, /* 17 ER AAC LC */
    0, /* 18 (Reserved) */
    0, /* 19 ER AAC LTP */
    0, /* 20 ER AAC scalable */
    0, /* 21 ER TwinVQ */
    0, /* 22 ER BSAC */
    0, /* 23 ER AAC LD */
    0, /* 24 ER CELP */
    0, /* 25 ER HVXC */
    0, /* 26 ER HILN */
    0, /* 27 ER Parametric */
    0, /* 28 (Reserved) */
    0, /* 29 (Reserved) */
    0, /* 30 (Reserved) */
    0  /* 31 (Reserved) */
};
#endif

/* Table 1.6.1 */
int FAADAPI AudioSpecificConfig(char *pBuffer, unsigned int buffer_size, mp4AudioSpecificConfig *mp4ASC)
{
    return AudioSpecificConfig2(pBuffer, buffer_size, mp4ASC, NULL);
}

#if 0
signed char FAADAPI AudioSpecificConfig2(char *pBuffer, unsigned int buffer_size, mp4AudioSpecificConfig *mp4ASC, program_config *pce)
{
    bitfile ld;
    signed char result = 0;

    if (pBuffer == NULL)
        return -7;
    if (mp4ASC == NULL)
        return -8;

    memset(mp4ASC, 0, sizeof(mp4AudioSpecificConfig));

    faad_initbits(&ld, pBuffer, buffer_size);
    faad_byte_align(&ld);

    mp4ASC->objectTypeIndex = (char)faad_getbits(&ld, 5);

    mp4ASC->samplingFrequencyIndex = (char)faad_getbits(&ld, 4);

    mp4ASC->channelsConfiguration = (char)faad_getbits(&ld, 4);

    mp4ASC->samplingFrequency = get_sample_rate(mp4ASC->samplingFrequencyIndex);

    if (ObjectTypesTable[mp4ASC->objectTypeIndex] != 1)
    {
        faad_endbits(&ld);
        return -1;
    }

    if (mp4ASC->samplingFrequency == 0)
    {
        faad_endbits(&ld);
        return -2;
    }

    if (mp4ASC->channelsConfiguration > 7)
    {
        faad_endbits(&ld);
        return -3;
    }

    /* get GASpecificConfig */
    if (mp4ASC->objectTypeIndex == 1 || mp4ASC->objectTypeIndex == 2 ||
        mp4ASC->objectTypeIndex == 3 || mp4ASC->objectTypeIndex == 4 ||
        mp4ASC->objectTypeIndex == 6 || mp4ASC->objectTypeIndex == 7)
    {
        result = GASpecificConfig(&ld, mp4ASC, pce);
    } else {
        result = -4;
    }

    faad_endbits(&ld);

    return result;
}
#else
//Shawn 2004.12.9
int FAADAPI AudioSpecificConfig2(char *pBuffer, unsigned int buffer_size, mp4AudioSpecificConfig *mp4ASC, program_config *pce)
{
    bitfile ld;
    signed char result = 0;

    if (pBuffer == NULL)
        return -7;
    if (mp4ASC == NULL)
        return -8;

    memset(mp4ASC, 0, sizeof(mp4AudioSpecificConfig));

    faad_initbits(&ld, pBuffer, buffer_size);
    faad_byte_align(&ld);

    mp4ASC->objectTypeIndex = (char)faad_getbits(&ld, 5);

    mp4ASC->samplingFrequencyIndex = (char)faad_getbits(&ld, 4);

    mp4ASC->channelsConfiguration = (char)faad_getbits(&ld, 4);

    mp4ASC->samplingFrequency = get_sample_rate(mp4ASC->samplingFrequencyIndex);

    /*if (ObjectTypesTable[mp4ASC->objectTypeIndex] != 1)
    {
        faad_endbits(&ld);
        return -1;
    }*///removed by Shawn 2004.12.9

    if (mp4ASC->samplingFrequency == 0)
    {
        faad_endbits(&ld);
        return -2;
    }

    if (mp4ASC->channelsConfiguration > 7)
    {
        faad_endbits(&ld);
        return -3;
    }

    /* get GASpecificConfig */
    if (mp4ASC->objectTypeIndex == 1 || mp4ASC->objectTypeIndex == 2 ||
        mp4ASC->objectTypeIndex == 3 || mp4ASC->objectTypeIndex == 4 ||
        mp4ASC->objectTypeIndex == 6 || mp4ASC->objectTypeIndex == 7)
    {
        result = GASpecificConfig(&ld, mp4ASC, pce);
    } else {
        result = -4;
    }

    faad_endbits(&ld);

    return result;
}
#endif
