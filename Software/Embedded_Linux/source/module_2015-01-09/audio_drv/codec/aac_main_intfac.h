/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The
** Material is protected by worldwide copyright and trade secret laws and treaty
** provisions. No part of the Material may be used, copied, reproduced, modified,
** published, uploaded, posted, transmitted, distributed, or disclosed in any way
** without FARADAY(MTD)/GRAIN prior express written permission.
**
** No license under any patent, copyright, trade secret or other intellectual
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
**
***********************************************************************/


/*******************************************************************************
	File:		aac_main_intfac.h

	Content:	aac encoder interface functions

*******************************************************************************/

#ifndef _aac_main_intfac_h_
#define _aac_main_intfac_h_


#include "typedef.h"
//#include "config.h"
#include "out_bitstrm.h"

#include "perceptual_cfg.h"
#include "perceptual_main.h"
#include "quantizer_main.h"

/*-------------------------- defines --------------------------------------*/


/*-------------------- structure definitions ------------------------------*/
typedef  struct {
  int32_t   sampleRate;            /* audio file sample rate */
  int16_t   nChannelsIn;           /* number of channels on input (1,2) */
  int16_t   bandWidth;             /* targeted audio bandwidth in Hz */
  int16_t   adtsUsed;			  /* whether write adts header */
  int16_t   nChannelsOut;          /* number of channels on output (1,2) */
  int32_t   bitRate;               /* encoder bit rate in bits/sec */
} GM_AAC_SPEC;


typedef struct {
  GM_PART_INFO elInfo;      /* int16_t size: 4 */

  GM_AAC_SPEC config;     /* int16_t size: 8 */

  GM_PERCEP_OUT    psyOut;        /* int16_t size: MAX_CHAN*186 + 2 = 188 / 374 */
  GM_PSY_BLK psyKernel;     /* int16_t size:  2587 / 4491 */

  GM_QC_BLK qcKernel;        /* int16_t size: 6 + 5(PADDING) + 7(GM_ELEMENT_BITS) + 54(GM_ADJ_THR_BLK) = 72 */
  GM_QC_OUTSTRM   qcOut;           /* int16_t size: MAX_CHAN*920(GM_QC_CHANNDATA_OUT) + 5(GM_QC_OUTSTRM_PART) + 7 = 932 / 1852 */

  struct GM_BITSTREAM_INFO bseInit; /* int16_t size: 6 */
  struct GM_STREAM_BITBUFMANAG  bitStream;            /* int16_t size: 8 */
  HANDLE_BIT_BUF  hBitStream;
  int			  initOK;

  int			enclen;
  int			inlen;
  short			*intbuf;
  short			*encbuf;
  short			*inbuf;

  void			*hCheck;

  int			intlen;
  int			uselength;

	int			checkid1;
	int			checkid2;
	int			checkid3;
	int			checkid4;
	int 		id;
}GM_AAC_ENCODER; /* int16_t size: 3809 / 6851 */

/*-----------------------------------------------------------------------------

functionname: AacDefault_InitConfig
description:  gives reasonable default configuration
returns:      ---

------------------------------------------------------------------------------*/
void AacDefault_InitConfig(GM_AAC_SPEC *config);

/*---------------------------------------------------------------------------

functionname:AacEnc_Init
description: allocate and initialize a new encoder instance
returns:     AACENC_OK if success

---------------------------------------------------------------------------*/




int16_t  AacEnc_Init (GM_AAC_ENCODER				*hAacEnc,       /* pointer to an encoder handle, initialized on return */
                    const  GM_AAC_SPEC     config);        /* pre-initialized config struct */

int16_t AacEncEncode_Main(GM_AAC_ENCODER		   *hAacEnc,
                    int16_t             *timeSignal,
                    const uint8_t       *ancBytes,      /*!< pointer to ancillary data bytes */
                    int16_t             *numAncBytes,   /*!< number of ancillary Data Bytes, send as fill element  */
                    uint8_t             *outBytes,      /*!< pointer to output buffer            */
                    uint32_t             *numOutBytes    /*!< number of bytes in output buffer */
                    );

/*---------------------------------------------------------------------------

functionname:AacEncClose
description: deallocate an encoder instance

---------------------------------------------------------------------------*/

void AacEncClose (GM_AAC_ENCODER* hAacEnc); /* an encoder handle */

#endif /* _aacenc_h_ */
