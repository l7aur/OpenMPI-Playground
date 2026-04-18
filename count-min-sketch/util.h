#pragma once

#include <mpi/mpi.h>

// #define CUSTOM_DEBUG

#ifdef CUSTOM_DEBUG
#define CUSTOM_DEBUG_SLAVE
#define CUSTOM_DEBUG_MASTER
#endif

#define __try int __error_code = 0;
#define __throw(X) { __error_code = (X); goto final; }
#define __finally final:

#define MASTER_FAILED_ERROR_CODE            1
#define SLAVE_FAILED_ERROR_CODE             2
#define INITIALIZATION_FAILED_ERROR_CODE    3
#define INPUT_FILE_PATH_MAX_SIZE            100
#define NUMBER_OF_SEEDS                     5
#define MPI_HASH_SEED                       MPI_UNSIGNED
#define MPI_SKETCH                          MPI_UNSIGNED
#define MIN(X, Y)                           (((X) < (Y)) ? (X) : (Y))

typedef int input_value;
typedef unsigned int hash_seed;
typedef unsigned int hash_value;
typedef struct _count_min_sketch {
    hash_value* data;
    hash_seed* seeds;
    int rows;
    int cols;
} count_min_sketch;

static const hash_seed SEEDS[NUMBER_OF_SEEDS] = {
    0x92d6a354, 0x8bf65351, 0x960b7a1f, 0x9d670b00, 0xb32d6bd1
};

int parse(
    const int argc,
    char* argv[],
    char input_file_path[INPUT_FILE_PATH_MAX_SIZE],
    int* number_of_hash_functions,
    int* number_of_counters
);

int do_task(
    const int id,
    int numbers[],
    const int number_of_numbers,
    const int number_of_counters,
    const int number_of_hash_functions,
    count_min_sketch* global_sketch /* OUT */
);

int count_min_sketch_create(
    const int number_of_counters,
    const int number_of_hash_functions,
    count_min_sketch* sketch /* OUT */
);

void count_min_sketch_free(
    count_min_sketch* sketch
);

void count_min_sketch_insert(
    count_min_sketch* sketch,
    const input_value x
);

int count_min_sketch_estimate_count(
    count_min_sketch* sketch,
    const input_value x
);

#if defined(CUSTOM_DEBUG) || defined(CUSTOM_DEBUG_MASTER) || defined(CUSTOM_DEBUG_SLAVE)
void count_min_sketch_print(
    const int id,
    count_min_sketch* sketch
);
#endif