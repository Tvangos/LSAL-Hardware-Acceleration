/* Parallelize with OMP */
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
	char *string1, char *string2,
	int *max_index, int *similarity_matrix, short *direction_matrix, int N, int M)
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

	char database_val;

	// Here the real computation starts.
    // k is the current diagonal. There are (M + N - 1) diagonals
    for (int k = 0; k < M + N - 1; k++) {
        
        // Calculate the starting and ending row indices for this diagonal
        int start_i = (k < N) ? 0 : k - N + 1;
        int end_i   = (k < M) ? k : M - 1;

        // Parallelize work alogn the diagonal
        #pragma omp parallel for default(none) \
		shared(similarity_matrix, direction_matrix, string1, string2, max_index, \
			max_value, k, start_i, end_i, N, M) private(test_val, val, dir, match)
        for (int i = start_i; i <= end_i; i++) {
            
			// If we know the diagonal and the row and column (j) is fixed
            int j = k - i; 
            int index = i * N + j;

            int north = 0, west = 0, northwest = 0;

            // Check boundaries
            bool hasNorth = (i > 0);
            bool hasWest  = (j > 0);

            if (hasNorth) north = similarity_matrix[index - N];
            if (hasWest)  west  = similarity_matrix[index - 1];
            if (hasNorth && hasWest) northwest = similarity_matrix[index - N - 1];

            match = (string1[j] == string2[i]) ? MATCH : MISS_MATCH;
            int val = 0;
            short dir = CENTER;

            // W
            int test_val = west + GAP_i;
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
				// Double check before updating
                #pragma omp critical
                { 
                    if (val > max_value) {
                        max_value = val;
                        *max_index = index;
                    }
                }
            }

        }
    }
}  // end of function

/************************************************************************/

/* 	Back-tracking stage 
	Returns an array with the output sequence backwards. */
void back_tracking(short *direction_matrix, char *out_seq, int *seq_length,
					int index, char *query, int N) {
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
			strcpy(out_seq + *seq_length, "-");
			index -= N;
		}
		// WEST
		else {
			strcpy(out_seq + *seq_length, "-");
			index -= 1;
		}
		(*seq_length)++;
	}
}

/* ******************************************************************/
int main(int argc, char** argv) {

    double t1, t2;

	if (argc != 3) {
		printf("%s <Query Size N> <DataBase Size M>\n", argv[0]);
		return EXIT_FAILURE;
	}
 
    printf("Starting Local Alignment Code \n");
	fflush(stdout);

	/* Typically, M >> N */
	int N = atoi(argv[1]); 
    int M = atoi(argv[2]);

    char *query = (char*) malloc(sizeof(char) * (N+1));
	char *database = (char*) malloc(sizeof(char) * (M+1));
	int *similarity_matrix = (int*) malloc(sizeof(int) * N * M);
	short *direction_matrix = (short*) malloc(sizeof(short) * N * M);
	int *max_index = (int *) malloc(sizeof(int));
	char *output_seq = (char *) malloc(sizeof(char) * (N + M + 1));
	int *seq_length = (int *) malloc(sizeof(int));

/* Create the two input strings by calling a random number generator */
	fillRandom(query, N);
	fillRandom(database, M);
	
	// strcpy(query, "TGTTACGG");
	// strcpy(database, "GGTTGACTA");

	// printf("Query: %s\n", query);
	// printf("Database: %s\n", database);

	memset(similarity_matrix, 0, sizeof(int) * N * M);
	memset(direction_matrix, 0, sizeof(short) * N * M);

    t1 = omp_get_wtime();
	compute_matrices(query, database, max_index, similarity_matrix, direction_matrix, N, M);
	t2 = omp_get_wtime();
	
	// printf("\n Similarity Matrix");
	// printf("\n==========================================================\n");
	// for(int i = 0; i < M; i++) {
	// 	for (int j = 0; j < N; j++) {
	// 		printf("%3d ", similarity_matrix[i*N + j]);
	// 	}
	// 	printf("\n");
	// }
	// printf("==========================================================\n");

	// printf("\n Direction Matrix");
	// printf("\n==========================================================\n");
	// for(int i = 0; i < M; i++) {
	// 	for (int j = 0; j < N; j++) {
	// 		print_dir(direction_matrix[i*N + j]);
	// 	}
	// 	printf("\n");
	// }
	// printf("==========================================================\n");

	*seq_length = 0;
	back_tracking(direction_matrix, output_seq, seq_length, *max_index, query, N);
	print_sequence(output_seq, *seq_length);

    printf(" max index is in position (%d, %d) \n", max_index[0]/N, max_index[0]%N );
	printf(" execution time of LSAL SW algorithm is %f sec \n", t2-t1);
	
	free(query);
	free(database);
	free(similarity_matrix);
	free(direction_matrix);
	free(max_index);
	free(output_seq);
	free(seq_length);

    // printf("cnt_ops=%ld, cnt_bytes=%ld\n", cnt_ops, cnt_bytes);
	return EXIT_SUCCESS;
}