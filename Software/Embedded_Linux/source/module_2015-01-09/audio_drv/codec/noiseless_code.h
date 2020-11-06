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
	File:		noiseless_code.h

	Content:	Noiseless coder module structure and functions

*******************************************************************************/

#ifndef _noiseless_code_h
#define _noiseless_code_h

#include "perceptual_const.h"
#include "tns_tool.h"
#include "huffman_1.h"



#define MAX_SECTIONS          MAX_GROUPED_SFB
#define SECT_ESC_VAL_LONG    31
#define SECT_ESC_VAL_SHORT    7
#define CODE_BOOK_BITS        4
#define SECT_BITS_LONG        5
#define SECT_BITS_SHORT       3

typedef struct
{
  int16_t codeBook;
  int16_t sfbStart;
  int16_t sfbCnt;
  int16_t sectionBits;
}
GM_SECTION_PARAM; 




typedef struct
{
  int16_t maxSfbPerGroup;
  int16_t sfbPerGroup;
  int16_t blockType;
  int16_t noOfGroups;
  int16_t sfbCnt;  
  int16_t noOfSections;
  GM_SECTION_PARAM sectionInfo[MAX_SECTIONS];  
  int16_t scalefacBits;             /* scalefac   coded bits */
  int16_t firstScf;                 /* first scf to be coded */
  int16_t sideInfoBits;             /* sectioning bits       */
  int16_t huffmanBits;              /* huffman    coded bits */  
  int16_t bitLookUp[MAX_SFB_LONG*(CODE_BOOK_ESC_NDX+1)];
  int16_t mergeGainLookUp[MAX_SFB_LONG];
}
GM_SECTION_DATA; /*  int16_t size: 10 + 60(MAX_SECTIONS)*4(GM_SECTION_PARAM) + 51(MAX_SFB_LONG)*12(CODE_BOOK_ESC_NDX+1) + 51(MAX_SFB_LONG) = 913 */


//int16_t BCInit(void);

int16_t NoiselessCoder(const int16_t *quantSpectrum,
                   const uint16_t *maxValueInSfb,
                   const int16_t *scalefac,
                   const int16_t blockType,
                   const int16_t sfbCnt,
                   const int16_t maxSfbPerGroup,
                   const int16_t sfbPerGroup,
                   const int16_t *sfbOffset,
                   GM_SECTION_DATA *sectionData);

#endif
