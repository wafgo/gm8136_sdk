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
	File:		typedef.h

	Content:	type defined for defferent paltform

*******************************************************************************/

#ifndef typedef_h
#define typedef_h "$Id $"



#undef ORIGINAL_TYPEDEF_H /* define to get "original" ETSI version
                             of typedef.h                           */

#ifdef ORIGINAL_TYPEDEF_H
/*
 * this is the original code from the ETSI file typedef.h
 */

#if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER) || defined(__ZTC__)
typedef signed char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef int Flag;

#elif defined(__sun)
typedef signed char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef int Flag;

#elif defined(__unix__) || defined(__unix)
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef int Flag;

#endif
#else /* not original typedef.h */

/*
 * use (improved) type definition file typdefs.h and add a "Flag" type
 */
#include "typedefs.h"
typedef int Flag;

#endif


//#define ReVer

#ifdef ReVer
#define    AacEnc_Init         AacEnc_11
#define    AacEncEncode_Main   AacEnc_12


#define Init_ChannelPart                 cc1
#define Init_ChannelPartInfo             cc2
#define Init_ChannelPartBits             cc3
#define Perceptual_AllocMem              cc4
#define Perceptual_DelMem                cc5
#define Perceptual_OutStrmSet0           cc6
#define Perceptual_OutStrmSetNULL        cc7
#define Perceptual_MainInit              cc8
#define Perceptual_AdvLngBlk             cc9
#define Perceptual_AdvLngBlkMS           cc11
#define Perceptual_AdvShortBlk           cc23
#define Perceptual_AdvShortBlkMS         cc14
#define Perceptual_Main                  cc15
#define Perce_InitCfgLngBlk              cc16
#define Psy_InitCfgShortBlk              cc17
#define Get_SampleRateIndex              cc18
#define Calc_MinSnrParm                  cc19
#define Start_BarkVal                    cc200
#define Calc_BarkOneLineVal              cc21
#define Init_EnergySpreading             cc22
#define SetStart_ThrQuiet                cc23
#define Cal_AtAppr                       cc24
#define AacDefault_InitConfig            cc25
#define Init_TnsCfgLongBlk               cc27
#define Init_TnsCfgShortBlk              cc29
#define Tns_FreqToBandWithRound          cc31
#define Tns_Filtering                    cc32
#define Tns_ParcorCoef2Index             cc33
#define Tns_Index2ParcorCoef             cc45
#define Tns_AnalysisFilter               cc56
#define Tns_CalcFiltDecide               cc55
#define Tns_SyncParam                    cc57
#define Tns_CalcLPCWeightedSpectrum      cc58
#define Tns_FIR                          cc59
#define Tns_ApplyMultTableToRatios       cc99
#define Tns_Search31                     cc100
#define Tns_Search32                     cc101
#define Tns_AutocorrelationToParcorCoef  cc110
#define Tns_CalcLPCFilter                cc111
#define Tns_AutoCorre                    cc112
#define MDCT_Trans                       cc113
#define MDCT_ShifDelayBuf                cc114
#define MDCT_CalcScalefactor             cc115
#define MDCT_BlkLong                     cc116
#define MDCT_BlkShort                    cc117
#define MDCT_One                         cc118
#define MDCT_Second                      cc119
#define Radix4_1                         cc1200
#define Radix8FFT                        cc121
#define ShuffleSwap                      cc133
#define ShortBlkGroup                    cc155
#define MidSideStereo                    cc156
#define UpdateOutParameter               cc157
#define BlockSwitch_Init                 cc158
#define DataBlockSwitching               cc159
#define CalcBlkEnergy                    cc160
#define SearchMax                        cc161
#define UpdateBlock                      cc162
#define QuantCode_Init                   cc163
#define QC_Main                          cc164
#define QC_ParmSetNULL                   cc165
#define QC_ParamMem_Init                 cc166
#define QC_ParamMem_Del                  cc167
#define QC_Mem0                          cc168
#define QC_WriteBitrese                  cc169
#define QC_WriteOutParam                 cc1180
#define UpdateBitrate                    cc190
#define SearchMaxSpectrun                cc12kk
#define EestimFrmLen                     cc564
#define CalcFrmPad                       cc00236
#define EstimFactAllChan                 cc89754
#define CalcScaleFactAllChan             cc962
#define EstFactorOneChann                cc9632
#define CalcScaleFactOneChann            cc562
#define BetterScalFact                   cc563
#define Calc_Sqrtdiv256                  cc564
#define Scf_CountDiffBit                 cc5555
#define Scf_SearchSingleBands            cc1100
#define Scf_DiffReduction                cc1110
#define Count_PeDiff                     cc1111
#define Count_SinglePe                   cc1112
#define CalcScf_SingleBitsHuffm          cc1113
#define ThrParam_Init                    cc1114
#define Calc_Thr                         cc1150
#define Calc_FactorSpendBitreservoir     cc1116
#define Perceptual_Entropy_Fact          cc123456
#define Calc_ThrToPe                     cc1151
#define LoudnessThrRedExp                cc1152
#define RedBandMinSnr                    cc1153
#define deterBandAvoiHole                cc1154
#define ReduThrFormula                   cc1155
#define SumPeAvoidHole                   cc1156
#define DiffPeThrSfBand                  cc1157
#define ReduPeSnr                        cc1158
#define MidSid_HoleChann                 cc1159
#define Conv_bitToPe                     cc1160
#define Save_DynBitThr                   cc1161
#define Calc_PeMinPeMax                  cc1162
#define BitSavePercen                    cc1163
#define Calc_SfbPe_First                 cc1164
#define calc_ConstPartSfPe               cc1169
#define QuantScfBandsSpectr              cc1170
#define SpectLinesQuant                  cc1171
#define SingLineQuant                    cc1172
#define iQuantL1                         cc1173
#define Quant_Line2Distor                cc1147
#define NoiselessCoder                   cc1179
#define Nl_CounBit                       cc1180
#define Nl_ScalefactorsCntBit            cc1181
#define Nl_BitCntHuffTable               cc1182
#define Codbooks_1                       cc1183
#define Codbooks_2                       cc1184
#define Codbooks_3                       cc1185
#define GoodCodBook                      cc1168
#define FindBigGain                      cc1188
#define CalcSectBitMaxBig                cc1189
#define FindBitBigTable                  cc1190
#define MinSecBigBits                    cc1120
#define SfbMdct_Energy                   cc2001
#define SfbMdct_MSEnergy                 cc2002
#define BuildNew_BitBuffer               cc2003
#define Del_BitBuf                       cc2004
#define Get_BitsCnt                      cc2005
#define Write_Bits2Buf                   cc2006
#define Set0Start_BitBufManag            cc2007
#define Alignm_mem                       cc2009
#define Aligmem_free                     cc2100
#define FindMax_EngerSpreading           cc2101
#define BitCountFunc                     cc2102
#define MsMaskBitsCnt                    cc2103
#define TnsBitCnt                        cc2104
#define TnsBitsDemand                    cc2105
#define OutEncoder_BitData               cc2106
#define OutSingleChannelBits             cc2107
#define OutPairChannelBits               cc2108
#define OutSubBlkPartBits                cc2109
#define TnsPartBits                      cc2110
#define IcsPartBits                      cc2111
#define MSPartBits                       cc2112
#define GainControlPartBits              cc2113
#define SpectrPartBits                   cc2114
#define SectionPartBits                  cc2115
#define ScaleFactorPartBits              cc2116
#define GlobalGainPartBits               cc2117
#define OutPersonalChannelBits           cc2118
#define Get_HuffTableScf                 cc2120
#define Write_HuffTabScfBitLen           cc2121
#define Write_HuffBits                   cc2122
#define Call_bitCntFunTab                cc122111
#define Write_LookUpTabHuf_Esc           cc2123
#define Write_LookUpTabHuf_0             cc2124
#define Write_LookUpTabHuf_1             cc2125
#define Write_LookUpTabHuf_2             cc2126
#define Write_LookUpTabHuf_3             cc2167
#define Write_LookUpTabHuf_4             cc2128
#define Write_LookUpTabHuf_5             cc2129

#define Div_32                           cc2130

//	check ID
#define check_id												 cc2131

//table encoder/decode
#define Encode_Table                     cccc1234
#define Init_Table                       ccccc1235


//table
#define ShortWindowSine                 dddd001
#define LongWindowKBD                   dd000
#define twidTab64                       dd002
#define twidTab512                      dd003
#define cossintab_128                   dd004
#define cossintab_1024                  dd005
#define bitrevTab_17                    dd006
#define bitrevTab_129                   dd007
#define formfac_sqrttable               dd008
#define mTab_3_4                        dd009
#define mTab_4_3                        dd010
#define pow2tominusNover16              dd011
#define specExpMantTableComb_enc        dd0012
#define specExpTableComb_enc            dd013
#define quantBorders                    dd014
#define quantRecon                      dd015
#define huff_ltab1_2                    dd016
#define huff_ltab3_4                    dd017
#define huff_ltab5_6                    dd018
#define huff_ltab7_8                    dd019
#define huff_ltab9_10                   dd020
#define huff_ltab11                     dd021
#define huff_ltabscf                    dd022
#define huff_ctab1                      dd023
#define huff_ctab2                      dd024
#define huff_ctab3                      dd025
#define huff_ctab4                      dd026
#define huff_ctab5                      dd027
#define huff_ctab6                      dd028
#define huff_ctab7                      dd029
#define huff_ctab8                      dd030
#define huff_ctab9                      dd031
#define huff_ctab10                     dd032
#define huff_ctab11                     dd033
#define huff_ctabscf                    dd040


#endif






#endif
