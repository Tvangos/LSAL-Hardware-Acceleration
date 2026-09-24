/********************************************************************************
 * The Host code running in the Arm CPU.
 ********************************************************************************/
#include <sys/time.h> 

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include "lsal.h"

const short GAP_i_sw = -1;
const short GAP_d_sw = -1;
const short MATCH_sw = 2;
const short MISS_MATCH_sw = -1;
const short CENTER_sw = 0;
const short NORTH_sw = 1;
const short NORTH_WEST_sw = 2;
const short WEST_sw = 3;

 /***************************************************************************************
  * This is the golden code which runs in the CPU (and is the same code that you developed for x86 / Arm) 
  * It will be used to verify the correct functionality of the HW implementation.  
  * Its usefulness is mainly when you perform software emulation.
  ***************************************************************************************/
void compute_matrices_sw(
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
	int dir = CENTER_sw;
	int match = 0;

	//Here the real computation starts. Place your code whenever is required. 
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
		
		match = ( string1[j] == string2[i] ) ? MATCH_sw : MISS_MATCH_sw;
		val = 0;

		// W
		test_val = west + GAP_i_sw;
		if(test_val >= val) {
			val = test_val;
			dir = WEST_sw;
		}

		// N
		test_val = north + GAP_d_sw;
		if(test_val >= val) {
			val = test_val;
			dir = NORTH_sw;
		}

		// NW
		test_val = northwest + match;
		if(test_val >= val) {
			val = test_val;
			dir = NORTH_WEST_sw;
		}

		// C
		if(val <= 0) {
			val = 0;
			dir = CENTER_sw;
		}
		else if (val > max_value) {
			*max_index = index;
			max_value = val;
		}
        
		similarity_matrix[index] = val;
		direction_matrix[index] = dir;
   }
}

/*
 Given an event, this function returns the kernel execution time in ms
 */
double getTimeDifference(cl_event event) {
	cl_ulong time_start = 0;
	cl_ulong time_end = 0;
	double total_time = 0.0f;

	clGetEventProfilingInfo(event,
	CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start,
	NULL);
	clGetEventProfilingInfo(event,
	CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end,
	NULL);
	total_time = time_end - time_start;
	return total_time / 1000000.0; // To convert nanoseconds to milliseconds
}


/*
 return a random number between 0 and limit inclusive.
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
void fillRandom(char* string, int dimension, int padding) {
    static const char possibleLetters[] = "ATCG";
    int i=0;

    if (padding) {
        for (i=0; i < padding; i++) {
            string[i] = 'P';
        }
    }

    for (i=padding; i < dimension+padding; i++) {
        int randomNum = rand_lim(3);
        string[i] = possibleLetters[randomNum];
    }

    if (padding) {
        for (i=dimension+padding; i < 2*padding+dimension; i++) {
            string[i] = 'P';
        }
    }

    string[dimension+2*padding] = '\0';
}
dna_t char_to_dna_t(char c) {
    switch(c) {
        case 'A': return ADENINE;
        case 'C': return CYTOSINE;
        case 'G': return GUANINE;
        case 'T': return THYMENE;
        case '-': return DASH;
        case 'P': return PAD;
        case '\0': return EOS;
        default: return EOS;
    }
}

char dna_t_to_char(dna_t val) {
    if (val == ADENINE)       return 'A';
    else if (val == CYTOSINE) return 'C';
    else if (val == GUANINE)  return 'G';
    else if (val == THYMENE)  return 'T';
    else if (val == DASH)     return '-';
    else if (val == PAD)      return 'P';
    else if (val == EOS)      return '\0';
    else                      return '?';
}
int load_file_to_memory(const char *filename, char **result) {
	size_t size = 0;
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		*result = NULL;
		return -1; // -1 means file opening fail
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *) malloc(size + 1);
	if (size != fread(*result, sizeof(char), size, f)) {
		free(*result);
		return -2; // -2 means file reading fail
	}
	fclose(f);
	(*result)[size] = 0;
	return size;
}

/*******************************************************************************
 *   Host program running on the Arm CPU. 
 *   The code is written using the OpenCL API. 
 *   We have provided multiple comments for you to understand where each thing  
*******************************************************************************/
int main(int argc, char** argv) {
	printf("starting HOST code \n");
	fflush(stdout);
	int err;                            // error code returned from api calls
	cl_uint matrix_size,dir_matrix_size,database_size, query_size;

    if (argc != 4) {
		printf("%s <input xclbin file> <Query Size N> <DataBase Size M>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
    cl_uint N = atoi(argv[2]); 
    cl_uint M = atoi(argv[3]);
    if (N <= 0 || M <= 0) {
    	printf("N and M should be positive numbers. \n");
		return EXIT_FAILURE;
	}

    matrix_size = (N+1)*(M+N+3);
    dir_matrix_size = N*M;
    database_size = (M + 2 * N);
    query_size = N;
    char *query_char = (char *) malloc(sizeof(char) * (N + 1));
    char *database_char = (char *) malloc(sizeof(char) * (database_size + 1));
	dna_t *query = (dna_t*) malloc(sizeof(dna_t) * query_size);
	dna_t *database = (dna_t*) malloc(sizeof(dna_t) * database_size);
	score_t *similarity_matrix = (score_t*) malloc(sizeof(score_t) * matrix_size);
	direction_t *direction_matrix = (direction_t*) malloc(sizeof(direction_t) * matrix_size);
	int *max_index = (int *) malloc(sizeof(int));

	printf("array defined! \n");
    fflush(stdout);

	cl_platform_id platform_id;         // platform id
	cl_device_id device_id;             // compute device id
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel

	char cl_platform_vendor[1001];
	char cl_platform_name[1001];

	cl_mem input_query;
	cl_mem input_database;
	cl_mem output_similarity_matrix;
	cl_mem output_direction_matrix;
	cl_mem output_max_index;
    // Convert to dna_t Enum types

    fillRandom(query_char, N, 0);
    fillRandom(database_char, M, N-1);
    for (cl_uint i = 0; i < N; i++) {
      query[i] = char_to_dna_t(query_char[i]);
    }
    for (cl_uint i = 0; i < database_size; i++) {
      database[i] = char_to_dna_t(database_char[i]);
    }
	for (cl_uint i = 0; i < matrix_size; i++) {
			similarity_matrix[i] = 0;
			direction_matrix[i] = 0;
		}

/**********************************************
 * 			Xilinx OpenCL Initialization
 *
 * We must follow specific steps to get the necessary
 * information and handlers, in order to be able
 * to use the available accelerator device (FPGA).
 * After every step, we always check for any errors
 * that might have occured. In case of error, the
 * program aborts and exits immediately.
 * *********************************************/

  /**************************************************
	* Step 1:
   * Get available OpenCL platforms and devices.
   * In our case, it is a Xilinx FPGA device.
   * If the underlying platform has other accelerators
   * available, we could use them too (e.g. GPU, CPU).
	**************************************************/
	printf("GET platform \n");
	err = clGetPlatformIDs(1, &platform_id, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to find an OpenCL platform!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("GET platform vendor \n");
	err = clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, 1000,
			(void *) cl_platform_vendor, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("CL_PLATFORM_VENDOR %s\n", cl_platform_vendor);
	printf("GET platform name \n");
	err = clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 1000,
			(void *) cl_platform_name, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: clGetPlatformInfo(CL_PLATFORM_NAME) failed!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("CL_PLATFORM_NAME %s\n", cl_platform_name);

	// Connect to a compute device
	//
	int fpga = 1;

	printf("get device FPGA is %d  \n", fpga);
	err = clGetDeviceIDs(platform_id,
			fpga ? CL_DEVICE_TYPE_ACCELERATOR : CL_DEVICE_TYPE_CPU, 1,
			&device_id, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to create a device group!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	/*********************************************
	 * Step 2 : Create Context
	 *********************************************/
	printf("create context \n");
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
	if (!context) {
		printf("Error: Failed to create a compute context!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

   /*********************************************
	 * Step 3 : Create Command Queue
	 *********************************************/
	printf("create queue \n");
	commands = clCreateCommandQueue(context, device_id,
	CL_QUEUE_PROFILING_ENABLE, &err);
	if (!commands) {
		printf("Error: Failed to create a command commands!\n");
		printf("Error: code %i\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	cl_int binary_status;

	/*********************************************
	 * Step 4 : Load Hardware Binary File (*.xclbin) from disk
	 **********************************************/
	unsigned char *kernelbinary;
	char *xclbin = argv[1];
	printf("loading %s\n", xclbin);
	int n_i = load_file_to_memory(xclbin, (char **) &kernelbinary);
	if (n_i < 0) {
		printf("failed to load kernel from xclbin: %s\n", xclbin);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

  /********************************************************
	* Step 5 : Create program using the loaded hardware binary file
	********************************************************/
	size_t n = n_i;
	// Create the compute program from offline
	printf("create program with binary \n");
	program = clCreateProgramWithBinary(context, 1, &device_id, &n,
			(const unsigned char **) &kernelbinary, &binary_status, &err);
	free(kernelbinary);

	if ((!program) || (err != CL_SUCCESS)) {
		printf("Error: Failed to create compute program from binary %d!\n",
				err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	/**************************************************************
	 *  Step 6 for 1 Compute Unit: Create Kernels - the actual handler of the kernel that
    *           we will be using. We first create a program, and then
    *           obtain the kernel handler from the program.
	 **************************************************************/
	printf("build program \n");
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		size_t len;
		char buffer[2048];

		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG,
				sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	// Create the compute kernel in the program we wish to run
	//
	printf("create kernel \n");
	kernel = clCreateKernel(program, "compute_matrices", &err);
	if (!kernel || err != CL_SUCCESS) {
		printf("Error: Failed to create compute kernel!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

    /**************************************************************-
    * Step 7 : Create buffers.
    * We do not need to allocate separate memory space (malloc), because
    * on a MPSoC system (e.g. ZCU102 board), we map the memory space that is
    * allocated at clCreateBuffer, to a usable memory space for our
    * host application. We also do not need to use free for any reason.
    * See Xilinx UG1393 for detailed information.
    **************************************************************/
    input_query = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(dna_t) * query_size, NULL, NULL);
    input_database = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(dna_t) * database_size, NULL, NULL);
    output_similarity_matrix = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(score_t) * matrix_size, NULL, NULL);
    output_direction_matrix = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(direction_t) * matrix_size, NULL, NULL);
    output_max_index = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, NULL);

	if (!input_query || !input_database || !output_similarity_matrix
			|| !output_direction_matrix || !output_max_index) {
		printf("Error: Failed to allocate device memory!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

   /**************************************************************
    * Step 8 : Write the Input Data to the Write Buffers of the device memory
    **************************************************************/
	err = clEnqueueWriteBuffer(commands, input_query, CL_TRUE, 0,
			sizeof(dna_t) * N, query, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, input_database, CL_TRUE, 0,
			sizeof(dna_t) * M, database, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, output_similarity_matrix, CL_TRUE, 0,
			sizeof(score_t) * matrix_size, similarity_matrix, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to write to source array a!\n");
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueWriteBuffer(commands, output_direction_matrix, CL_TRUE, 0,
				sizeof(direction_t) * matrix_size, direction_matrix, 0, NULL, NULL);
		if (err != CL_SUCCESS) {
			printf("Error: Failed to write to source array a!\n");
			printf("Test failed\n");
			return EXIT_FAILURE;
		}

	/**************************************************************
	 * Step 9: Set the arguments to our compute kernel
	 **************************************************************/
	err = 0;
	printf("set arg 0 \n");
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_query);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 0! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 1 \n");
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_database);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 1! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 2 \n");
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_max_index);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 2! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 3 \n");
	err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &output_similarity_matrix);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 3! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 4 \n");
	err |= clSetKernelArg(kernel, 4, sizeof(cl_mem),
			&output_direction_matrix);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 4! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 5 \n");
	err |= clSetKernelArg(kernel, 5, sizeof(cl_uint),
			&N);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 5! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	printf("set arg 6 \n");
	err |= clSetKernelArg(kernel, 6, sizeof(cl_uint),
			&M);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to set kernel arguments 6! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	
	/**************************************************************
	 * Step 10: Place the Kernel in the Queue for Execution
	 **************************************************************/
/**************************************************************
     * Step 10: Place the Kernel in the Queue for Execution
     **************************************************************/
    cl_event enqueue_kernel;
    struct timeval start_time, end_time; // 1. Define the timers

    printf("LAUNCH task \n");
    
    gettimeofday(&start_time, NULL); // 2. Start the clock!

    err = clEnqueueTask(commands, kernel, 0, NULL, &enqueue_kernel);

    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        printf("Test failed\n");
        return EXIT_FAILURE;
    }
    
    clWaitForEvents(1, &enqueue_kernel); // Wait for the hardware to finish

    gettimeofday(&end_time, NULL); // 3. Stop the clock!

	/**************************************************************
	 * Step 11: Read back the results from the device to verify the output
	 **************************************************************/
	cl_event readMax, readSimilarity, readDirections;
	err = clEnqueueReadBuffer(commands, output_similarity_matrix, CL_TRUE, 0,
			sizeof(score_t) * matrix_size, similarity_matrix, 0, NULL, &readSimilarity);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	err = clEnqueueReadBuffer(commands, output_direction_matrix, CL_TRUE, 0,
			sizeof(direction_t) * matrix_size, direction_matrix, 0, NULL,
			&readDirections);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}
	err = clEnqueueReadBuffer(commands, output_max_index, CL_TRUE, 0,
			sizeof(int), max_index, 0, NULL, &readMax);
	if (err != CL_SUCCESS) {
		printf("Error: Failed to read array! %d\n", err);
		printf("Test failed\n");
		return EXIT_FAILURE;
	}

	clWaitForEvents(1, &readSimilarity);
	clWaitForEvents(1, &readDirections);
	clWaitForEvents(1, &readMax);

	// Calculate difference in milliseconds
    double executionTime = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                           (end_time.tv_usec - start_time.tv_usec) / 1000.0;

   clReleaseMemObject(input_database);
	clReleaseMemObject(input_query);
	clReleaseMemObject(output_direction_matrix);
	clReleaseMemObject(output_max_index);
	clReleaseMemObject(output_similarity_matrix);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

/**************************************************************
 * Run the same algorithm in the Host Unit and compare for verification
 **************************************************************/
	int * similarity_matrix_sw = ( int *) malloc(
			sizeof( int) * matrix_size);
	short * direction_matrix_sw = ( short*) malloc(
			sizeof( short) * matrix_size);
	int * max_index_sw = ( int *) malloc(
				sizeof( int));

	for(cl_uint i = 0; i < matrix_size; i++){
		similarity_matrix_sw[i] = 0;
	}
	compute_matrices_sw(query_char, database_char, max_index_sw, similarity_matrix_sw, direction_matrix_sw, N, M);

	printf("both ended\n");

    direction_t *unpacked_dir_matrix = (direction_t *)malloc(sizeof(direction_t) * M * N);
    for (cl_uint i = 0; i < dir_matrix_size; i++) {
        unpacked_dir_matrix[i] = 0;
    }

    // Unpacking of the direction matrix to match the 2D structure of the SW version for easier comparison
    for (cl_uint i = 2; i <= M + N; i++) {
        for (cl_uint j = 1; j < N + 1; j++) {
            int row = i - j - 1;
            int col = j - 1;
            
            if (row >= 0 && row < M) {
                unpacked_dir_matrix[row * N + col] = direction_matrix[i * (N + 1) + j];
            }
        }
    }

	printf(" execution time is %lf ms \n", executionTime);
	for (cl_uint i = 0; i < dir_matrix_size; i++) {
		if (direction_matrix_sw[i] != unpacked_dir_matrix[i]) {
			printf("Error, mismatch in the results, i + %d, SW: %d, HW %d \n",
					i, direction_matrix_sw[i], (int)unpacked_dir_matrix[i]);
			return EXIT_FAILURE;
		}
	}

	printf("computation ended!- RESULTS CORRECT \n");

	/**************************************************************
	 * Clean up everything and, then, shutdown 
	 **************************************************************/
    
   free(query);
   free(database);
   free(similarity_matrix);
	free(direction_matrix);
	free(max_index);
	free(similarity_matrix_sw);
	free(direction_matrix_sw);
	free(max_index_sw);
	free(unpacked_dir_matrix);

	return EXIT_SUCCESS;
}

