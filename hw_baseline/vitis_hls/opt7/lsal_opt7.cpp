#include "lsal.h"
#include "ap_int.h"
#include <string.h>

#define MAX_N 32
#define MAX_M 65536
#define MAX_DATABASE_SIZE (MAX_M + 2 * MAX_N)

extern "C" {
void compute_matrices(
      dna_t *query,            
      dna_t *database,         
      int *max_index,
      ap_uint<64> *direction_matrix_packed)
{
    int local_max_index = 0;
    score_t global_max_value = 0;

    #pragma HLS INTERFACE m_axi port=direction_matrix_packed bundle=gmem0 offset=slave
    #pragma HLS INTERFACE m_axi port=max_index               bundle=gmem1 offset=slave
    #pragma HLS INTERFACE m_axi port=query                   bundle=gmem2 offset=slave
    #pragma HLS INTERFACE m_axi port=database                bundle=gmem3 offset=slave

    #pragma HLS INTERFACE s_axilite port=direction_matrix_packed bundle=control
    #pragma HLS INTERFACE s_axilite port=max_index               bundle=control
    #pragma HLS INTERFACE s_axilite port=query                   bundle=control
    #pragma HLS INTERFACE s_axilite port=database                bundle=control
    #pragma HLS INTERFACE s_axilite port=return                  bundle=control

    dna_t query_buffer[MAX_N];
    dna_t database_buffer[MAX_DATABASE_SIZE];

    #pragma HLS ARRAY_PARTITION variable=query_buffer complete dim=1
    #pragma HLS ARRAY_PARTITION variable=database_buffer cyclic factor=32 dim=1

    memcpy(query_buffer, query, MAX_N * sizeof(dna_t));
    memcpy(database_buffer, database, MAX_DATABASE_SIZE * sizeof(dna_t));

    score_t row_curr[MAX_N+1];
    score_t row_prev[MAX_N+1];
    score_t row_old[MAX_N+1];
    ap_uint<2> dir_array[MAX_N+1];

    #pragma HLS ARRAY_PARTITION variable=row_curr complete dim=1
    #pragma HLS ARRAY_PARTITION variable=row_prev complete dim=1
    #pragma HLS ARRAY_PARTITION variable=row_old  complete dim=1
    #pragma HLS ARRAY_PARTITION variable=dir_array complete dim=1

    INIT_LOOP: for(int j = 0; j <= MAX_N; j++) {
        #pragma HLS UNROLL
        row_curr[j] = 0;
        row_prev[j] = 0;
        row_old[j]  = 0;
        dir_array[j] = CENTER;
    }

    dna_t local_db_window[MAX_N];
    #pragma HLS ARRAY_PARTITION variable=local_db_window complete dim=1

    PREFILL_WINDOW: for(int k=0; k<MAX_N; k++) {
        #pragma HLS PIPELINE II=1
        local_db_window[k] = database_buffer[MAX_N - 1 - k];
    }

    OUTSIDE_LOOP: for (int i = 2; i < MAX_M+MAX_N+3; i++) {
        #pragma HLS PIPELINE II=2
        score_t row_max_value = 0;
        int row_max_index = 0;
        ap_uint<64> packed_row_dirs = 0;

        INSIDE_LOOP: for (int j = 1; j <= MAX_N; j++) {
            score_t north     = row_prev[j];
            score_t west      = row_prev[j-1];
            score_t northwest = row_old[j-1];

            score_t match = (query_buffer[j-1] == local_db_window[j-1]) ? MATCH : MISS_MATCH;

            score_t score_w  = west + GAP_d;
            score_t score_n  = north + GAP_i;
            score_t score_nw = northwest + match;

            score_t best_score = 0;
            ap_uint<2> best_dir = CENTER;

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

            row_curr[j] = best_score;
            dir_array[j] = best_dir;
            
            // Pack 2-bit direction into 64-bit word
            packed_row_dirs.range((j*2)-1, (j-1)*2) = best_dir;
        }

        FIND_MAX_LOOP: for (int j = 1; j <= MAX_N; j++) {
            #pragma HLS UNROLL
            if (row_curr[j] > row_max_value) {
                row_max_value = row_curr[j];
                row_max_index = i * (MAX_N + 1) + j;
            }
        }

        direction_matrix_packed[i] = packed_row_dirs;

        if (row_max_value > global_max_value) {
            global_max_value = row_max_value;
            local_max_index = row_max_index;
        }

        SHIFT_HISTORY: for (int j = 0; j <= MAX_N; j++) {
            #pragma HLS UNROLL
            row_old[j]  = row_prev[j];
            row_prev[j] = row_curr[j];
        }

        SHIFT_DB_WINDOW: for(int k = MAX_N-1; k > 0; k--) {
            #pragma HLS UNROLL
            local_db_window[k] = local_db_window[k-1];
        }

        if ((i + MAX_N - 2) < MAX_DATABASE_SIZE) {
            local_db_window[0] = database_buffer[i + MAX_N - 2];
        } else {
            local_db_window[0] = 0;
        }
    }

    *max_index = local_max_index;
}
}
