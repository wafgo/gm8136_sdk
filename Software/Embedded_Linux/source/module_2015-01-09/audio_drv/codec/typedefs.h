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



#ifndef typedefs_h
#define typedefs_h "$Id $"
#include <linux/module.h>
#include <linux/kernel.h>

#define ARM_INASM


#ifndef CHAR_BIT
#define CHAR_BIT      8         /* number of bits in a char */
#endif

#ifndef GMAAC_SHRT_MAX
#define GMAAC_SHRT_MAX    (32767)        /* maximum (signed) short value */
#endif

#ifndef GMAAC_SHRT_MIN
#define GMAAC_SHRT_MIN    (-32768)        /* minimum (signed) short value */
#endif

/* Define NULL pointer value */
#ifndef NULL
#define NULL    ((void *)0)
#endif

#define INT_BITS   32

/*
********************************************************************************
*                         DEFINITION OF CONSTANTS
********************************************************************************
*/
/*
 ********* define char type
 */
typedef char Char;

#ifndef min
#define min(a,b) ( a < b ? a : b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a : b)
#endif

#define ARMV4_INASM		1

#define ARMV5TE_SAT           1
#define ARMV5TE_ADD           1
#define ARMV5TE_SUB           1
#define ARMV5TE_SHL           1
#define ARMV5TE_SHR           1
#define ARMV5TE_L_SHL         1
#define ARMV5TE_L_SHR         1

//basic operation functions optimization flags
#define SATRUATE_IS_INLINE              1   //define saturate as inline function
#define SHL_IS_INLINE                   1  //define shl as inline function
#define SHR_IS_INLINE                   1   //define shr as inline function
#define L_MULT_IS_INLINE                1   //define L_mult as inline function
#define L_MSU_IS_INLINE                 1   //define L_msu as inline function
#define L_SUB_IS_INLINE                 1   //define L_sub as inline function
#define L_SHL_IS_INLINE                 1   //define L_shl as inline function
#define L_SHR_IS_INLINE                 1   //define L_shr as inline function
#define ADD_IS_INLINE                   1   //define add as inline function //add, inline is the best
#define SUB_IS_INLINE                   1   //define sub as inline function //sub, inline is the best
#define DIV_S_IS_INLINE                 1   //define div_s as inline function
#define MULT_IS_INLINE                  1   //define mult as inline function
#define NORM_S_IS_INLINE                1   //define norm_s as inline function
#define NORM_L_IS_INLINE                1   //define norm_l as inline function
#define ROUND_IS_INLINE                 1   //define round as inline function
#define L_MAC_IS_INLINE                 1   //define L_mac as inline function
#define L_ADD_IS_INLINE                 1   //define L_add as inline function
#define EXTRACT_H_IS_INLINE             1   //define extract_h as inline function
#define EXTRACT_L_IS_INLINE             1   //define extract_l as inline function        //???
#define MULT_R_IS_INLINE                1   //define mult_r as inline function
#define SHR_R_IS_INLINE                 1   //define shr_r as inline function
#define MAC_R_IS_INLINE                 1   //define mac_r as inline function
#define MSU_R_IS_INLINE                 1   //define msu_r as inline function
#define L_SHR_R_IS_INLINE               1   //define L_shr_r as inline function

#define	GMID_AUDIO_BASE				 0x42000000							/*!< The base param ID for AUDIO codec */
#define	GMID_AUDIO_FORMAT			(GMID_AUDIO_BASE | 0X0001)		/*!< The format data of audio in track */
#define	GMID_AUDIO_SAMPLEREATE		(GMID_AUDIO_BASE | 0X0002)		/*!< The sample rate of audio  */
#define	GMID_AUDIO_CHANNELS			(GMID_AUDIO_BASE | 0X0003)		/*!< The channel of audio */
#define	GMID_AUDIO_BITRATE			(GMID_AUDIO_BASE | 0X0004)		/*!< The bit rate of audio */
#define GMID_AUDIO_CHANNELMODE		(GMID_AUDIO_BASE | 0X0005)		/*!< The channel mode of audio */

#define	ERR_AUDIO_BASE				0x82000000
#define ERR_AUDIO_UNSCHANNEL		ERR_AUDIO_BASE | 0x0001
#define ERR_AUDIO_UNSSAMPLERATE		ERR_AUDIO_BASE | 0x0002
#define ERR_AUDIO_UNSFEATURE		ERR_AUDIO_BASE | 0x0003


#define ERR_NONE						0x00000000
#define ERR_FINISH						0x00000001
#define ERR_BASE						0X80000000
#define ERR_FAILED						0x80000001
#define ERR_OUTOF_MEMORY				0x80000002
#define ERR_NOT_IMPLEMENT				0x80000003
#define ERR_INVALID_ARG					0x80000004
#define ERR_INPUT_BUFFER_SMALL			0x80000005
#define ERR_OUTPUT_BUFFER_SMALL			0x80000006
#define ERR_WRONG_STATUS				0x80000007
#define ERR_WRONG_PARAM_ID				0x80000008


typedef struct {
	uint8_t*	Buffer;		/*!< Buffer pointer */
	int64_t	    Time;		/*!< The time of the buffer */
	uint32_t	Length;		/*!< Buffer size in byte */
} GM_AUDIO_BUF;

typedef void* AAC_PTR;
typedef void* AAC_HANDLE;







#endif
