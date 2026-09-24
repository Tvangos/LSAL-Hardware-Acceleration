#include "lsal.h"

direction_t *order_matrix_blocks(direction_t *in_matrix) {
    // MATRIX_SIZE_DIRECTION is M * N
    direction_t *out_matrix = (direction_t *)malloc(sizeof(direction_t) * MATRIX_SIZE_DIRECTION);

    int num_diag = 0;
    int store_elem = 1;

    // Loop over every diagonal
    for(num_diag = 0; num_diag < M + N - 1; num_diag++) {
        int tmp_i, tmp_j;

        // Determine starting row/col for the current diagonal
        if(num_diag < M) {
            tmp_j = num_diag;  // row
            tmp_i = 0;         // col
        } else {
            tmp_j = M - 1;                 // row
            tmp_i = num_diag - M + 1;      // col
        }

        // Walk up the diagonal
        for(int j = 0; j < store_elem; j++) {
            // Write index for dense 2D matrix
            int store_index = tmp_j * N + tmp_i;

            // Project 2D coordinates back to Hardware Systolic Coordinates
            int hw_i = tmp_j + tmp_i + 2;   // HW time step (i)
            int hw_j = tmp_i + 1;           // HW Processing Element (j)

            // Read index from the padded hardware array (Stride N+1)
            int read_index = hw_i * (N + 1) + hw_j;

            out_matrix[store_index] = in_matrix[read_index];

            // Move up and right along the diagonal
            tmp_j--;
            tmp_i++;
        }

        // Update the number of active elements in the diagonal
        if(num_diag < N - 1) {
            store_elem++;       // Pipeline filling
        } else if (num_diag >= M - 1) {
            store_elem--;       // Pipeline emptying
        }
    }

    return out_matrix;
}