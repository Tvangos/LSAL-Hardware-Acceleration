#include "lsal.h"
#include <string.h>

int compute_top_level(
      score_t similarity_matrix[MATRIX_SIZE],
      direction_t direction_matrix[MATRIX_SIZE],
      dna_t query[N],
      dna_t database[DATABASE_SIZE],
      int *max_index,
      dna_t output_seq[N + M + 1])
{
    int index, local_max_index = 0;
    score_t global_max_value = 0;

    dna_t query_buffer[N];
    dna_t database_buffer[DATABASE_SIZE];

    memcpy(query_buffer, query, N*sizeof(dna_t));
    memcpy(database_buffer, database, DATABASE_SIZE*sizeof(dna_t));

    score_t circular_array[4][N+1];
    direction_t dir_array[N+1];

    int curr = 2;
    int prev = 1;
    int old  = 0;

    INIT_LOOP_OUTSIDE:for(int k=0; k < 4; k++) {
        #pragma HLS PIPELINE
        INIT_LOOP_INSIDE:for(int j=0; j < N+1; j++) {

            circular_array[k][j] = 0;
        }
    }

    OUTSIDE_LOOP: for (int i = 2; i < M+N+3; i++) {
        score_t row_max_value = 0;
        int row_max_index = 0;
        int curr = i % 4;
        int prev = (i - 1) % 4;
        int old  = (i - 2) % 4;


        INSIDE_LOOP: for (int j = 1; j < N+1; j++) {
            index = i * (N + 1) + j;

            score_t north     = circular_array[prev][j];
            score_t west      = circular_array[prev][j-1];
            score_t northwest = circular_array[old][j-1];

            score_t match = (query_buffer[j-1] == database_buffer[i-j+N-2]) ? MATCH : MISS_MATCH;

            score_t score_w  = west + GAP_d;
            score_t score_n  = north + GAP_i;
            score_t score_nw = northwest + match;

            score_t best_score = 0;
            direction_t best_dir = CENTER;

            // West
            if (score_w >= best_score) {
                best_score = score_w;
                best_dir = WEST;
            }

            // North
            if (score_n >= best_score) {
                best_score = score_n;
                best_dir = NORTH;
            }

            // North-West 
            if (score_nw >= best_score) {
                best_score = score_nw;
                best_dir = NORTH_WEST;
            }

            if (best_score <= 0) {
                best_score = 0;
                best_dir = CENTER;
            }

            circular_array[curr][j] = best_score;
            dir_array[j] = best_dir;

            if (best_score > row_max_value) {
                row_max_value = best_score;
                row_max_index = index;
            }
        }

        if (row_max_value > global_max_value) {
            global_max_value = row_max_value;
            local_max_index = row_max_index;
        }


        memcpy(&similarity_matrix[(N+1)*i], circular_array[curr], (N+1)*sizeof(score_t));
        memcpy(&direction_matrix[(N+1)*i], dir_array, (N+1)*sizeof(direction_t));
    }

    *max_index = local_max_index;
    return 0;
}
