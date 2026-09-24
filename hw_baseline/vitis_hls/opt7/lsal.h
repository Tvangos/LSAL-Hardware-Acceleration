#ifndef LSAL_H_
#define LSAL_H_

#define N 32
#define M 65536

#define MATRIX_SIZE ((N+1)*(M+N+3))
#define DATABASE_SIZE (M + 2 * N)
#define MATRIX_SIZE_DIRECTION (M * N)

#include "ap_int.h"
#include <string.h>

typedef ap_uint<2> direction_t;  	// 8 bits (Values 0-3)
typedef short score_t;              // 16 bits signed (Values -32768 to 32767)
typedef unsigned char dna_t;        // 8 bits (Values 0-6)
typedef unsigned char bool_t;

const score_t GAP_i = -1;
const score_t GAP_d = -1;
const score_t MATCH = 2;
const score_t MISS_MATCH = -1;

const direction_t CENTER = 0;
const direction_t NORTH = 1;
const direction_t NORTH_WEST = 2;
const direction_t WEST = 3;

const dna_t THYMENE = 0;
const dna_t ADENINE = 1;
const dna_t CYTOSINE = 2;
const dna_t GUANINE = 3;
const dna_t DASH = 4;
const dna_t EOS = 5;
const dna_t PAD = 6;


extern "C" {
	int compute_top_level(score_t similarity_matrix[MATRIX_SIZE],
      					  direction_t direction_matrix[MATRIX_SIZE],
      					  dna_t query[N],
      					  dna_t database[DATABASE_SIZE],
      					  int *max_index,
      					  dna_t output_seq[N + M + 1]);
}

#endif

