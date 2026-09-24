#ifndef LSAL_H
#define LSAL_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define N 32
#define M 65536

const short GAP_i = -1;
const short GAP_d = -1;
const short MATCH = 2;
const short MISS_MATCH = -1;
const short CENTER = 0;
const short NORTH = 1;
const short NORTH_WEST = 2;
const short WEST = 3;

int compute_top_level(int *similarity_matrix, short *direction_matrix, char *query, \
					  char *database, int *max_index, char *output_seq);
#endif
