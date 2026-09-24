// Anti-diagonal traversal
#include "lsal.h"

/**********************************************************************************************
 * LSAL algorithm 
 * Inputs:
 *          string1 is the query[N]
 *          string2 is the database[M]
 *          input sizes N, M
 * Outputs:
 *           max_index is the location of the highest similiarity score 
 *           similarity and direction matrices. Note that these two matrices are initialized with zeros.
 **********************************************************************************************/

void compute_matrices(
	dna_t *string1, dna_t *string2,
	int *max_index, score_t similarity_matrix[N*(M+2*(N-1))], direction_t direction_matrix[N*(M+2*(N-1))])
{

    int index = 0;
    int i = 0;
	int j = 0;

	// Following values are used for the N, W, and NW values wrt. similarity_matrix[i]
    score_t north = 0;
	score_t west = 0;
	score_t northwest = 0;
	score_t max_value = 0;

	score_t val = 0;
	score_t test_val = 0;
	direction_t dir = CENTER;
	score_t match = 0;
	
	// k current anti-diagonal 
    OUTER_LOOP: for (int k = 0; k < M + N - 1; k++) {
		#pragma HLS loop_flatten off
        #pragma HLS PIPELINE off
    	INNER_LOOP: for (int i = k, j = N-1 ; j >= 0; i++, j--) {
			#pragma HLS PIPELINE
            index = i * N + j;

            north = 0;
			west = 0; 
			northwest = 0;

            // Check boundaries
            bool hasNorth = (i > 0);
            bool hasWest  = (j > 0);

            if (hasNorth) north = similarity_matrix[index - N];
            if (hasWest)  west  = similarity_matrix[index - 1];
            if (hasNorth && hasWest) northwest = similarity_matrix[index - N - 1];

            match = (string1[j] == string2[i]) ? MATCH : MISS_MATCH;
            val = 0;
            dir = CENTER;

            // W
            test_val = west + GAP_i;
            if (test_val >= val) {
                val = test_val;
                dir = WEST;
            }

            // N
            test_val = north + GAP_d;
            if (test_val >= val) {
                val = test_val;
                dir = NORTH;
            }

            // NW
            test_val = northwest + match;
            if (test_val >= val) {
                val = test_val;
                dir = NORTH_WEST;
            }

            // C
            if (val <= 0) {
                val = 0;
                dir = CENTER;
            } 
            
            similarity_matrix[index] = val;
            direction_matrix[index] = dir;

            if (val > max_value) {
                max_value = val;
                *max_index = index;
            }
      
        }
    }

}  // end of function

/************************************************************************/

int compute_top_level(score_t *similarity_matrix, direction_t *direction_matrix, dna_t *query, \
					  dna_t *database, int *max_index) {

	compute_matrices(query, database, max_index, similarity_matrix, direction_matrix);

	return 0;
}

