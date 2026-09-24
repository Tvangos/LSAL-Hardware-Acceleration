// Bursting memory accesses
#include "lsal.h"
#include "math.h"

int compute_top_level(score_t similarity_matrix[MATRIX_SIZE], direction_t direction_matrix[MATRIX_SIZE], \
                    dna_t query[N], dna_t database[DATABASE_SIZE], int *max_index, \
                    dna_t output_seq[N + M + 1]) {

    int index, local_max_index = 0;
    
    score_t north = 0;
    score_t west = 0;
    score_t northwest = 0;
    score_t max_value = 0;

    score_t val = 0;
    direction_t dir = CENTER;
    score_t match = 0;
    score_t score_w, score_n, score_nw;
    score_t best_score;
    direction_t best_dir;

    score_t global_max_value = 0;
    
    score_t row_sim[N+1];
    direction_t row_dir[N+1];

    dna_t query_buffer[N];
    dna_t database_buffer[DATABASE_SIZE];

    memcpy(query_buffer, query, N*sizeof(dna_t));
    memcpy(database_buffer, database, DATABASE_SIZE*sizeof(dna_t));
    
    OUTER_LOOP: for (int i = 2; i < M+N+1; i++) {
        score_t row_max_value = 0;
        int row_max_index = 0;
        INNER_LOOP: for (int j = 1; j < N+1; j++) {

            index = i * (N + 1) + j;
            north = similarity_matrix[index-N - 1];
            west = similarity_matrix[index-N - 2];
            northwest= similarity_matrix[index - (2*N) - 3]; 
            
            match = (query_buffer[j-1] == database_buffer[i-j+N-2]) ? MATCH : MISS_MATCH;
            score_w  = west + GAP_d;
            score_n  = north + GAP_i;
            score_nw = northwest + match;

            best_score = 0;
            best_dir = CENTER;

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

            val = best_score;
            dir = best_dir;
           
            row_sim[j] = val;
            row_dir[j] = dir;

            if (val > row_max_value) {
                row_max_value = val;
                row_max_index = index; 
            }
        }
        if (row_max_value > global_max_value) {
            global_max_value = row_max_value;
            local_max_index = row_max_index; 
        }

        memcpy(&similarity_matrix[(N+1)*i], row_sim, (N+1)*sizeof(score_t));
        memcpy(&direction_matrix[(N+1)*i], row_dir, (N+1)*sizeof(direction_t));
    }

    *max_index = local_max_index;

    return 0;
}