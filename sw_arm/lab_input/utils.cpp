#include "lsal.h"

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

void print_dir(short val) {
	switch (val)
	{
	case CENTER:
		printf(" C ");
		break;
	case NORTH:
		printf(" N ");
		break;
	case NORTH_WEST:
		printf("NW ");
		break;
	case WEST:
		printf(" W ");
		break;
	default:
		break;
	}
}

// print sequence in correct order
void print_sequence(char *out_seq, int length) {
	
	printf("Sequence: ");
	for (int i=length-1; i >= 0; i--) {
		printf("%c", out_seq[i]);
	}
	printf("\n");
}