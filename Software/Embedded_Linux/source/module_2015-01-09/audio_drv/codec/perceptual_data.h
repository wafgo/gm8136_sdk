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
	File:		perceptual_data.h

	Content:	Psychoacoustic data and structures

*******************************************************************************/

#ifndef _perceptual_data_h
#define _perceptual_data_h

#include "perceptual_tool.h"
#include "tns_tool.h"

/*
  the structs can be implemented as unions
*/

typedef struct{
  int32_t sfbLong[MAX_GROUPED_SFB];
  int32_t sfbShort[TRANS_FAC][MAX_SFB_SHORT];
}GM_SFB_THRSHD; /* int16_t size: 260 */

typedef struct{
  int32_t sfbLong[MAX_GROUPED_SFB];
  int32_t sfbShort[TRANS_FAC][MAX_SFB_SHORT];
}GM_SFB_ENGY; /* int16_t size: 260 */

typedef struct{
  int32_t sfbLong;
  int32_t sfbShort[TRANS_FAC];
}GM_SFB_ENGY_SUM; /* int16_t size: 18 */


typedef struct{
  GM_BLK_SWITCHING_CNT    blockSwitchingControl;          /* block switching */
  int16_t                    *mdctDelayBuffer;               /* mdct delay buffer [BLOCK_SWITCHING_OFFSET]*/
  int32_t                    sfbThresholdnm1[MAX_SFB];       /* PreEchoControl */
  int16_t                    mdctScalenm1;                   /* scale of last block's mdct (PreEchoControl) */

  GM_SFB_THRSHD             sfbThreshold;                   /* adapt           */
  GM_SFB_ENGY                sfbEnergy;                      /* sfb Energy      */
  GM_SFB_ENGY                sfbEnergyMS;  
  GM_SFB_ENGY_SUM            sfbEnergySumMS;
  GM_SFB_ENGY_SUM            sfbEnergySum;
  GM_SFB_ENGY                sfbSpreadedEnergy;
  
  int16_t                    mdctScale;                      /* scale of mdct   */
  int32_t                    *mdctSpectrum;                  /* mdct spectrum [FRAME_LEN_LONG] */  
}GM_PSY_PART_DATA; /* int16_t size: 4 + 87 + 102 + 360 + 360 + 360 + 18 + 18 + 360 = 1669 */

#endif /* _perceptual_data_h */
