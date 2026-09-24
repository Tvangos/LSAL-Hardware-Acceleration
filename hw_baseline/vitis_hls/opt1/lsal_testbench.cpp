#include "lsal.h"
#include <stdio.h>
#include <stdlib.h>

void compute_matrices_golden(
	char *string1, char *string2,
	int *max_index, int *similarity_matrix, short *direction_matrix)
{

    int index = 0;
    int i = 0;
	int j = 0;

    // Following values are used for the N, W, and NW values wrt. similarity_matrix[i]
    int north = 0;
	int west = 0;
	int northwest = 0;
	int max_value = 0;

	int val = 0;
	int test_val = 0;
	int dir = CENTER;
	int match = 0;

	// Here the real computation starts. Place your code whenever is required. 
	// Scan the N*M array row-wise starting from the second row.
   for(index = 0; index < N*M; index++) {

		j = index % N; // column index
	  	i = index / N; // row index

		// Check boundaries 
		bool hasNorth = (i > 0); // we are not at column 0
		bool hasWest  = (j > 0); // we are not at row 0

		// Assign values based on those boundaries
		north     = hasNorth ? similarity_matrix[index - N] : 0; 
		west      = hasWest  ? similarity_matrix[index - 1] : 0;
		northwest = (hasNorth && hasWest) ? similarity_matrix[index - N - 1] : 0;
		
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

/*
 return a random number in [0, limit].
 */
int rand_lim(int limit) {

	int divisor = RAND_MAX / (limit + 1);
	int retval;

	do {
		retval = rand() / divisor;
	} while (retval > limit);

	return retval;
}

/*
 Fill the string with random values
 */
void fillRandom(char* string, int dimension) {
	//fill the string with random letters..
	static const char possibleLetters[] = "ATCG";

	string[0] = '-';

	int i;
	for (i = 0; i < dimension; i++) {
		int randomNum = rand_lim(3);
		string[i] = possibleLetters[randomNum];
	}
	string[dimension] = '\0';
}

/* 	Back-tracking stage 
	Returns an array with the output sequence backwards. */
void back_tracking(short *direction_matrix, char *out_seq, int *seq_length,
					int index, char *query) {
	int j;
	short dir;
	
	while(1) {
		dir = direction_matrix[index];
		if (dir == CENTER) {
			j = index % N;
			break;
		}
		else if(dir == NORTH_WEST) {
			j = index % N;
			out_seq[*seq_length] = query[j];
			index -= (N + 1);
		} 
		else if (dir == NORTH) {
			out_seq[*seq_length] = '-';
			index -= N;
		}
		// WEST
		else {
			out_seq[*seq_length] = '-';
			index -= 1;
		}
		(*seq_length)++;
	}
}

void back_tracking_hw(direction_t direction_matrix[N*M], dna_t *out_seq, int *seq_length,
					int index, dna_t *query) {
	int j;
	direction_t dir;
	
	while(1) {
		dir = direction_matrix[index];
		if (dir == CENTER) {
			j = index % N;
			break;
		}
		else if(dir == NORTH_WEST) {
			j = index % N;
			out_seq[*seq_length] = query[j];
			index -= (N + 1);
		} 
		else if (dir == NORTH) {
			out_seq[*seq_length] = DASH;
			index -= N;
		}
		// WEST
		else {
			out_seq[*seq_length] = DASH;
			index -= 1;
		}
		(*seq_length)++;
	}
}


void reverse_sequence(int seq_length, char *output_seq) {
    
    for (int i = 0; i < seq_length / 2; i++) {
        char temp = output_seq[i];
        output_seq[i] = output_seq[seq_length - 1 - i];
        output_seq[seq_length - 1 - i] = temp;
    }
}

dna_t char_to_dna_t(char c) {
	switch(c) {
		case 'A': return ADENINE;
		case 'C': return CYTOSINE;
		case 'G': return GUANINE;
		case 'T': return THYMENE;
		case '-': return DASH;
		case '\0': return EOS;
		default: return EOS; // End of string or invalid character
	}
}

char dna_t_to_char(dna_t val) {
    if (val == ADENINE)       return 'A';
    else if (val == CYTOSINE) return 'C';
    else if (val == GUANINE)  return 'G';
    else if (val == THYMENE)  return 'T';
    else if (val == DASH)     return '-';
    else if (val == EOS)      return '\0';
    else                      return '?'; // Default fallback
}


int main() {
    int retval = 0, i;
    clock_t t1, t2;

    char *query_char = (char *) malloc(sizeof(char) * (N+1));
	char *database_char = (char *) malloc(sizeof(char) * (M+1));
    
    int *similarity_matrix_int = (int *) malloc(sizeof(int) * N * M);
	short *direction_matrix_short = (short *) malloc(sizeof(short) * N * M);

    dna_t *query = (dna_t *) malloc(sizeof(dna_t) * (N+1));
	dna_t *database = (dna_t *) malloc(sizeof(dna_t) * (M+1));
	score_t *similarity_matrix = (score_t *) malloc(sizeof(score_t) * N * M);
	direction_t *direction_matrix = (direction_t *) malloc(sizeof(direction_t) * N * M);
	int *max_index = (int *) malloc(sizeof(int));
	dna_t *output_seq = (dna_t *) malloc(sizeof(dna_t) * (N + M + 1));
	char *output_seq_char = (char *) malloc(sizeof(char) * (N + M + 1));
    int *seq_length = (int *) malloc(sizeof(int));

	char *output_seq_gold = (char *) malloc(sizeof(char) * (N + M + 1));
    int *seq_length_gold = (int *) malloc(sizeof(int));

    printf("Starting Local Alignment Code \n");

    /* Create the two input strings by calling a random number generator */
    fillRandom(query_char, N);
	fillRandom(database_char, M);
    
    for (i=0; i<N; i++){  
      query[i] = char_to_dna_t(query_char[i]);
    }
    for (i=0; i<M; i++){
      database[i] = char_to_dna_t(database_char[i]);
   }

    memset(similarity_matrix_int, 0, sizeof(int) * N * M);
	memset(direction_matrix_short, 0, sizeof(short) * N * M);

    /**** Create golden output ****/
    t1 = clock();
	compute_matrices_golden(query_char, database_char, max_index, similarity_matrix_int, direction_matrix_short);
	t2 = clock();
    
    *seq_length_gold = 0;
	back_tracking(direction_matrix_short, output_seq_gold, seq_length_gold, *max_index, query_char);
	reverse_sequence(*seq_length_gold, output_seq_gold);

    printf(" max index for golden is in position (%d, %d) \n", max_index[0]/N, max_index[0]%N );
	printf(" execution time of LSAL golden algorithm is %f sec \n", (double)(t2-t1) / CLOCKS_PER_SEC);

    /**** Run on hardware ****/
    memset(similarity_matrix, 0, sizeof(score_t) * N * M);
	memset(direction_matrix, 0, sizeof(direction_t) * N * M);
    *max_index = 0;

    t1 = clock();
    compute_top_level(similarity_matrix, direction_matrix, query, database, max_index, output_seq);
    t2 = clock();

    double exec_time = (double)(t2-t1) / CLOCKS_PER_SEC;
    printf(" max index is in position (%d, %d) \n", max_index[0]/N, max_index[0]%N );
    printf("execution time of LSAL HW algorithm is %f sec \n", exec_time);

    *seq_length = 0;
	back_tracking_hw(direction_matrix, output_seq, seq_length, *max_index, query);
    
    for (i=0; i < *seq_length; i++) {
        output_seq_char[i] = dna_t_to_char(output_seq[i]);
    }
    reverse_sequence(*seq_length, output_seq_char);

    // Compare results
    if (*seq_length != *seq_length_gold) {
        printf("Test FAILED: Output sequence legnths differ!\n");
    }
    else {
        int passed = 1;
        for (i = 0; i < *seq_length; i++) {
            if (output_seq_char[i] != output_seq_gold[i]) {
                printf("Test FAILED!\n");
                passed = 0;
                break;
            }
        }
        if(passed) {
            printf("Test PASSED!!\n");
        }
    }

    free(query);
	free(database);
	free(similarity_matrix);
	free(direction_matrix);
	free(max_index);
	free(output_seq);
	free(seq_length);

    return 0;
}