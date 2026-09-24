// Arbitrary data types
#include "lsal.h"

/**********************************************************************************************
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
	int *max_index, score_t similarity_matrix[N*M], direction_t direction_matrix[N*M])
{
	int index;
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

	// Here the real computation starts. Place your code whenever is required.
	// Scan the N*M array row-wise starting from the second row.
   	for(index = 0; index < N*M; index++) {
		#pragma HLS PIPELINE off
		j = index % N; // column index
	  	i = index / N; // row index

		// Check boundaries 
		bool hasNorth = (i > 0); // we are not at column 0
		bool hasWest  = (j > 0); // we are not at row 0

		// Assign values based on those boundaries
		north     = hasNorth ? similarity_matrix[index - N] : (score_t) 0; 
		west      = hasWest  ? similarity_matrix[index - 1] : (score_t) 0;
		northwest = (hasNorth && hasWest) ? similarity_matrix[index - N - 1] : (score_t) 0;
		
		match = ( string1[j] == string2[i] ) ? MATCH : MISS_MATCH;
		val = 0;

		// W
		test_val = west + GAP_i;
		if(test_val >= val) {
			val = test_val;
			dir = WEST;
		}

		// N
		test_val = north + GAP_d;
		if(test_val >= val) {
			val = test_val;
			dir = NORTH;
		}

		// NW
		test_val = northwest + match;
		if(test_val >= val) {
			val = test_val;
			dir = NORTH_WEST;
		}

		// C
		if(val <= 0) {
			val = 0;
			dir = CENTER;
		}
		else if (val > max_value) {
			*max_index = index;
			max_value = val;
		}
			
		similarity_matrix[index] = val;
		direction_matrix[index] = dir;
		
	}   // end of for-loop

}  // end of function

/************************************************************************/
int compute_top_level(score_t *similarity_matrix, direction_t *direction_matrix, dna_t *query, \
					  dna_t *database, int *max_index, dna_t *output_seq) {

	compute_matrices(query, database, max_index, similarity_matrix, direction_matrix);

	return 0;
}

