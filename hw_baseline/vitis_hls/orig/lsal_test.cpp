#include "lsal.h"

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

	int i;
	for (i = 0; i < dimension; i++) {
		int randomNum = rand_lim(3);
		string[i] = possibleLetters[randomNum];
	}
	string[dimension] = '\0';
}

void compute_matrices_init(int *similarity_matrix, short *direction_matrix) {
	int i;
	for(i = 0; i < N*M; i++) {
		similarity_matrix[i] = 0;
		direction_matrix[i] = 0;
	}
}

/* 	Back-tracking stage 
	Returns an array with the output sequence backwards. */
void back_tracking(short *direction_matrix, char *out_seq, int *seq_length,
					int index, char *query) {
	int i, j;
	short dir;
	
	while(1) {
		dir = direction_matrix[index];
		i = index % N;
		j = index / N;

		if (dir == CENTER) {
			break;
		}
		else if(dir == NORTH_WEST) {
			out_seq[*seq_length] = query[j];
			(*seq_length)++;

			// Boundery check
			if (i == 0 || j == 0) break;
			index -= (N + 1);
		} 
		else if (dir == NORTH) {
			out_seq[*seq_length] = '-';
			(*seq_length)++;

			// Boundery check
			if(i == 0) break;
			index -= N;
		}
		// WEST
		else {
			out_seq[*seq_length] = '-';
			(*seq_length)++;

			// Boundery check
			if(j == 0) break;
			index -= 1;
		}
	}
}

void reverse_sequence(int seq_length, char *output_seq) {
    
    for (int i = 0; i < seq_length / 2; i++) {
        char temp = output_seq[i];
        output_seq[i] = output_seq[seq_length - 1 - i];
        output_seq[seq_length - 1 - i] = temp;
    }
}

int main() {
    int retval = 0, i;
    clock_t t1, t2;

    char *query = (char*) malloc(sizeof(char) * (N+1));
	char *database = (char*) malloc(sizeof(char) * (M+1));
	int *similarity_matrix = (int*) malloc(sizeof(int) * N * M);
	short *direction_matrix = (short*) malloc(sizeof(short) * N * M);
	int *max_index = (int *) malloc(sizeof(int));
	char *output_seq = (char *) malloc(sizeof(char) * (N + M + 1));
	int *seq_length = (int *) malloc(sizeof(int));

	char *output_seq_gold = (char *) malloc(sizeof(char) * (N + M + 1));
    int *seq_length_gold = (int *) malloc(sizeof(int));

    printf("Starting Local Alignment Code \n");

    /* Create the two input strings by calling a random number generator */
    fillRandom(query, N);
	fillRandom(database, M);

    memset(similarity_matrix, 0, sizeof(int) * N * M);
	memset(direction_matrix, 0, sizeof(short) * N * M);

    /**** Create golden output ****/
    t1 = clock();
	compute_matrices_golden(query, database, max_index, similarity_matrix, direction_matrix);
	t2 = clock();
    
    *seq_length_gold = 0;
	back_tracking(direction_matrix, output_seq_gold, seq_length_gold, *max_index, query);
	reverse_sequence(*seq_length_gold, output_seq_gold);

    printf(" max index for golden is in position (%d, %d) \n", max_index[0]/N, max_index[0]%N );
	printf(" execution time of LSAL golden algorithm is %f sec \n", (double)(t2-t1) / CLOCKS_PER_SEC);

    /**** Run on hardware ****/
    memset(similarity_matrix, 0, sizeof(int) * N * M);
	memset(direction_matrix, 0, sizeof(short) * N * M);
    *max_index = 0;

    t1 = clock();
    compute_top_level(similarity_matrix, direction_matrix, query, database, max_index, output_seq);
    t2 = clock();

    double exec_time = (double)(t2-t1) / CLOCKS_PER_SEC;
    printf(" max index is in position (%d, %d) \n", max_index[0]/N, max_index[0]%N );
    printf("execution time of LSAL HW algorithm is %f sec \n", exec_time);

    *seq_length = 0;
	back_tracking(direction_matrix, output_seq, seq_length, *max_index, query);
    reverse_sequence(*seq_length, output_seq);

    // Compare results
    if (*seq_length != *seq_length_gold) {
        printf("Test FAILED: Output sequence legnths differ!\n");
    }
    else {
        int passed = 1;
        for (i = 0; i < *seq_length; i++) {
            if (output_seq[i] != output_seq_gold[i]) {
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