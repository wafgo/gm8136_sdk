#ifndef _QUANT_MATRIX_H_
	#define _QUANT_MATRIX_H_
	#include "portab.h"

	#define SOURCE_FMT_SZ 5
	#ifdef QUANT_MATRIX_GLOBAL
		#define QUANT_MATRIX_EXT
		const int16_t vop_width[SOURCE_FMT_SZ] = {128, 176, 352, 704, 1408};
		const int16_t vop_height[SOURCE_FMT_SZ] = {96, 144, 288, 576, 1152};
	#else
		#define QUANT_MATRIX_EXT extern
		extern const int16_t vop_width[SOURCE_FMT_SZ];
		extern const int16_t vop_height[SOURCE_FMT_SZ];
	#endif
//	QUANT_MATRIX_EXT const uint8_t default_intra_matrix[64];
//	QUANT_MATRIX_EXT const uint8_t default_inter_matrix[64];
	QUANT_MATRIX_EXT const uint8_t * get_default_intra_matrix(void);
	QUANT_MATRIX_EXT const uint8_t * get_default_inter_matrix(void);
	QUANT_MATRIX_EXT uint8_t set_quant_matrix(
					const uint8_t * cus_matrix,
					uint8_t * codec_matrix,
					const uint8_t * default_matrix,
					uint8_t * cus_flag,
					uint32_t * local_matrix);
	QUANT_MATRIX_EXT void init_quant_matrix(const uint8_t * cus_matrix,
					uint8_t * codec_matrix,
					uint32_t * local_matrix);
#endif							/* _QUANT_MATRIX_H_ */
