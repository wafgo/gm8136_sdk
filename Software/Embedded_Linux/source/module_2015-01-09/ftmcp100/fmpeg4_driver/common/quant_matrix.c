#define QUANT_MATRIX_GLOBAL
#include "quant_matrix.h"
#include "portab.h"

//#define FIX(X) (1 << 16) / (X) + 1
#define FIX1(X) (((((1 << 17) / (X) + 1) & 0x1FFFF) << 8) | X)

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
/*****************************************************************************
 * Local data
 ****************************************************************************/

static const uint8_t default_intra_matrix[64] = {
	8, 17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45
};
static const uint8_t default_inter_matrix[64] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
};
// added for transposed matrix convenience
const uint8_t transposed_order[64] = {
  0,  8, 16, 24, 32, 40, 48, 56,
  1,  9, 17, 25, 33, 41, 49, 57,
  2, 10, 18, 26, 34, 42, 50, 58,
  3, 11, 19, 27, 35, 43, 51, 59,
  4, 12, 20, 28, 36, 44, 52, 60,
  5, 13, 21, 29, 37, 45, 53, 61,
  6, 14, 22, 30, 38, 46, 54, 62,
  7, 15, 23, 31, 39, 47, 55, 63
};
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
const uint8_t *
get_default_intra_matrix(void)
{
	return default_intra_matrix;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
const uint8_t * 
get_default_inter_matrix(void)
{
	return default_inter_matrix;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
uint8_t 
set_quant_matrix(const uint8_t * cus_matrix,
			uint8_t * codec_matrix,
			const uint8_t * default_matrix,
			uint8_t * cus_flag,
			uint32_t * local_matrix)
{
	uint8_t i, change = 0;
//	uint8_t * default_matrix = default_intra_matrix;
	const uint8_t * transposed = transposed_order;

	*cus_flag = 0;
	for (i = 64; i > 0; i --, default_matrix ++, cus_matrix ++, codec_matrix ++, transposed ++) {
		if (*default_matrix != *cus_matrix)
			*cus_flag = 1;
		if (*codec_matrix != *cus_matrix) {
			*codec_matrix = *cus_matrix;
			// Roger did not transpose the matrix for hardware's convenience, so we
			// transpose it ....
			local_matrix[*transposed] = (uint32_t) FIX1(*codec_matrix);
			change = 1;
		}
	}
	return change;
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )

#if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
void 
init_quant_matrix(const uint8_t * cus_matrix,
			uint8_t * codec_matrix,
			uint32_t * local_matrix)
{
	uint8_t i;
	const uint8_t * transposed = transposed_order;

	for (i = 64; i > 0; i --, cus_matrix ++, codec_matrix ++, transposed ++) {
		*codec_matrix = *cus_matrix;
		// Roger did not transpose the matrix for hardware's convenience, so we
		// transpose it ....
		local_matrix[*transposed] = (uint32_t) FIX1(*codec_matrix);
	}
}
#endif // #if ( defined(ONE_P_EXT) || defined(TWO_P_EXT) )
