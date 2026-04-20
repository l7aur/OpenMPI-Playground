#include "util.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int parse(
    const int argc,
    char* argv[],
    char input_file_path[INPUT_FILE_PATH_MAX_SIZE],
    int* number_of_hash_functions,
    int* number_of_counters
)
{
    if (argc != 4) {
        fprintf(stderr, "Usage ./main <number_of_hash_functions> <number_of_counters> <input_file_path>\n");
        fprintf(stderr, "input file path size < %d", INPUT_FILE_PATH_MAX_SIZE);
        fprintf(stderr, "<number_of_hash_functions> must be >= 3 and <= 5\n");
        fprintf(stderr, "<number_of_counters> must be > 0\n");
        return EXIT_FAILURE;
    }

    *number_of_hash_functions = atoi(argv[1]);
    if (*number_of_hash_functions < 3 || *number_of_hash_functions > 5) {
        fprintf(stderr, "<number_of_hash_functions> must be >= 3 and <= 5\n");
        return EXIT_FAILURE;
    }

    *number_of_counters = atoi(argv[2]);
    if (*number_of_counters < 1) {
        fprintf(stderr, "<number_of_counters> must be > 0\n");
        return EXIT_FAILURE;
    }
    strncpy(input_file_path, argv[3], INPUT_FILE_PATH_MAX_SIZE - 1);
    input_file_path[INPUT_FILE_PATH_MAX_SIZE - 1] = '\0';  

    return EXIT_SUCCESS;
}

hash_value _MurmurHash(
    const hash_seed seed,
    const  input_value x
)
{
    assert(seed >= 0);

    hash_value key = x;
    key *= 0xCC9E2D51;
    key = (key << 15) | (key >> (32 - 15));
    key *= 0x1B873593;

    hash_value h = seed ^ key;
    h = (h << 13) | (h >> (32 - 13));
    h *= 5;
    h += 0xE6546B64;
    h ^= sizeof(unsigned int);
    h ^= (h >> 16);
    h *= 0x85EBCA6B;
    h ^= (h >> 13);
    h *= 0xC2B2AE35;
    h ^= (h >> 16);

    return h;
}

int _GenerateHashSeeds(
    const int number_of_hash_functions,
    hash_seed functions[]
)
{
    assert(number_of_hash_functions >= 3 && number_of_hash_functions <= 5);
    assert(number_of_hash_functions <= NUMBER_OF_SEEDS);
    assert(functions != NULL);

    for (int i = 0; i < number_of_hash_functions; i++)
        functions[i] = SEEDS[i];

    return EXIT_SUCCESS;
}

int do_task(
    const int id,
    int numbers[],
    const int number_of_numbers,
    const int number_of_counters,
    const int number_of_hash_functions,
    count_min_sketch* global_sketch
)
{
    assert(numbers != NULL);
    assert(number_of_numbers > 0);
    count_min_sketch local_sketch;
    memset(&local_sketch, 0, sizeof(count_min_sketch));

    __try {
        int status = count_min_sketch_create(number_of_counters, number_of_hash_functions, &local_sketch);
        if (status < 0 || local_sketch.data == NULL || local_sketch.seeds == NULL) {
            fprintf(stderr, "[%d] Failed to initialize local sketch!", id);
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < number_of_numbers; i++)
            count_min_sketch_insert(&local_sketch, numbers[i]);

        assert(
            (id == 0 && global_sketch != NULL && global_sketch->data != NULL
            && global_sketch->cols == local_sketch.cols && global_sketch->rows == local_sketch.rows)
            || 
            (id != 0 && global_sketch == NULL)
        );
        status = MPI_Reduce(
            local_sketch.data,
            id == 0 ? global_sketch->data : NULL,
            local_sketch.cols * local_sketch.rows,
            MPI_SKETCH,
            MPI_SUM,
            0,
            MPI_COMM_WORLD
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to call reduce!\n", id);
            __throw(EXIT_FAILURE);
        }
    }
    __finally {
#ifdef CUSTOM_DEBUG
        count_min_sketch_print(id, &local_sketch);
#elifdef CUSTOM_DEBUG_MASTER
        if (id == 0) count_min_sketch_print(id, &local_sketch);
#elifdef CUSTOM_DEBUG_SLAVE
        if (id != 0) count_min_sketch_print(id, &local_sketch);
#endif

#if defined(CUSTOM_DEBUG) || defined(CUSTOM_DEBUG_MASTER)
        if (id == 0) {
            fprintf(stdout, "[%d] Result:\n", id);
            count_min_sketch_print(id, global_sketch);
        }
#endif

        count_min_sketch_free(&local_sketch);

        if (__error_code != EXIT_SUCCESS)
            return __error_code;
    }

    return EXIT_SUCCESS;
}

int count_min_sketch_create(
    const int number_of_counters,
    const int number_of_hash_functions,
    count_min_sketch *sketch
)
{
    assert(sketch != NULL);
    assert(number_of_counters > 0);
    assert(number_of_hash_functions > 0);

    __try {
        sketch->cols = number_of_counters;
        sketch->rows = number_of_hash_functions;
        sketch->data = (hash_value*)malloc(sizeof(hash_value) * sketch->cols * sketch->rows);
        if (sketch->data == NULL) {
            fprintf(stderr, "Failed to allocate memory for count_min_sketch data!\n");
            __throw(EXIT_FAILURE);
        }
        memset(sketch->data, 0, sizeof(hash_value) * sketch->cols * sketch->rows);
        
        sketch->seeds = (hash_seed*)malloc(sizeof(hash_seed) * sketch->rows);
        if (sketch->seeds == NULL) {
            fprintf(stderr, "Failed to allocate memory for count_min_sketch seeds!\n");
            __throw(EXIT_FAILURE);
        }

        int status = _GenerateHashSeeds(sketch->rows, sketch->seeds);
        if (status < 0) {
            fprintf(stderr, "Failed to allocate memory for count_min_sketch seeds!\n");
            __throw(EXIT_FAILURE);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            count_min_sketch_free(sketch);
            return __error_code;
        }
    }

    return EXIT_SUCCESS;
}

void count_min_sketch_free(
    count_min_sketch* sketch
)
{
    assert(sketch != NULL);

    if (sketch->data != NULL)
        free(sketch->data), sketch->data = NULL;

    if (sketch->seeds != NULL)
        free(sketch->seeds), sketch->seeds = NULL;
}

void count_min_sketch_insert(
    count_min_sketch *sketch,
    const input_value x
)
{
    assert(sketch != NULL);
    assert(sketch->cols > 0 && sketch->rows > 0);
    assert(sketch->data != NULL && sketch->seeds != NULL);

    for (int h = 0; h < sketch->rows; h++) {
        unsigned int location = h * sketch->cols + _MurmurHash(sketch->seeds[h], x) % sketch->cols;
        sketch->data[location]++;
    }
}

int count_min_sketch_estimate_count(
    count_min_sketch *sketch,
    const input_value x
)
{
    assert(sketch != NULL);

    hash_value estimation = 0;
    for (int h = 0; h < sketch->rows; h++) {
        unsigned int location =  h * sketch->cols + _MurmurHash(sketch->seeds[h], x) % sketch->cols;
        estimation = MIN(estimation, sketch->data[location]);
    }

    return estimation;
}

#if defined(CUSTOM_DEBUG) || defined(CUSTOM_DEBUG_MASTER) || defined(CUSTOM_DEBUG_SLAVE)
void count_min_sketch_print(
    const int id,
    count_min_sketch* sketch
)
{
    assert(sketch != NULL);
    assert(sketch->data != NULL);
    assert(sketch->seeds != NULL);

    fprintf(stdout, "[%d] count min sketch: rows[%d] cols[%d]\n[%d] Data: ", id, sketch->rows, sketch->cols, id);
    for (int i = 0; i < sketch->rows; i++, fprintf(stdout, "| "))
        for (int j = 0; j < sketch->cols; j++)
            fprintf(stdout, "%d ", sketch->data[i * sketch->rows + j]);
    fprintf(stdout, "\n[%d] HashSeeds: ", id);
    for (int i = 0; i < sketch->rows; i++)
        fprintf(stdout, "%x ", sketch->seeds[i]);
    fprintf(stdout, "\n");
}
#endif