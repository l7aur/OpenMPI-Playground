#include "util.h"
#include "slave.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <mpi/mpi.h>

int _RetrieveSeedValue(
    const int id,
    hash_seed* seed
);

int _RetrieveNumbers(
    const int id,
    int** numbers,
    int number_of_numbers
);

int _RetrieveNumberOfNumbers(
    const int id,
    int* number_of_numbers
);

int slave(
    const int id,
    const int number_of_hash_functions,
    const int number_of_counters
)
{
    assert(id != 0);

    int* numbers = NULL, number_of_numbers = 0;

    __try {
        // start of execution
        int status = MPI_Barrier(MPI_COMM_WORLD);
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to set barrier!\n", id);
            __throw(status);
        }
        
        status = _RetrieveNumberOfNumbers(id, &number_of_numbers);
        if (status < 0) {
            fprintf(stderr, "[%d] Retrieving seed value failed!\n", id);
            __throw(SLAVE_FAILED_ERROR_CODE);
        }

        status = _RetrieveNumbers(id, &numbers, number_of_numbers);
        if (status < 0) {
            fprintf(stderr, "[%d] Retrieving numbers failed!\n", id);
            __throw(SLAVE_FAILED_ERROR_CODE);
        }

        status = do_task(id, numbers, number_of_numbers, number_of_counters, number_of_hash_functions, NULL);
        if (status < 0) {
            fprintf(stderr, "[%d] Failed to do task!\n", id);
            __throw(SLAVE_FAILED_ERROR_CODE);
        }
        // end of execution
        status = MPI_Barrier(MPI_COMM_WORLD);
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to unset barrier!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (numbers != NULL)
            free(numbers), numbers = NULL, number_of_numbers = 0;
        if (__error_code != EXIT_SUCCESS)
            return __error_code;
    }

    return EXIT_SUCCESS;
}

int _RetrieveSeedValue(
    const int id,
    hash_seed* seed
)
{
    assert(seed != NULL);

    __try {
        int status = MPI_Scatter(
            NULL,
            1,
            MPI_HASH_SEED,
            seed,
            1,
            MPI_HASH_SEED,
            0,
            MPI_COMM_WORLD
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to receive seed value!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS)
            return __error_code;
    }

    return EXIT_SUCCESS;
}

int _RetrieveNumberOfNumbers(
    const int id,
    int* number_of_numbers
)
{
    assert(number_of_numbers != NULL);

    __try {
        int status = MPI_Scatterv(
            NULL,
            NULL,
            NULL,
            MPI_INT,
            number_of_numbers,
            1,
            MPI_INT,
            0,
            MPI_COMM_WORLD
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to receive number of numbers!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS)
            return __error_code;
    }

    return EXIT_SUCCESS;
}

int _RetrieveNumbers(
    const int id,
    int** numbers,
    int number_of_numbers
)
{
    assert(numbers != NULL && *numbers == NULL);
    assert(number_of_numbers > 0);

    __try {
        *numbers = (int*)malloc(sizeof(int) * number_of_numbers);
        if (numbers == NULL) {
            fprintf(stderr, "[%d] Failed to allocate the numbers buffer!\n", id);
            __throw(EXIT_FAILURE);
        }

        int status = MPI_Scatterv(
            NULL,
            NULL,
            NULL,
            MPI_INT,
            *numbers,
            number_of_numbers,
            MPI_INT,
            0,
            MPI_COMM_WORLD
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to receive numbers!\n", id);
            __throw(status);
        }
    }
    __finally {
#ifdef CUSTOM_DEBUG
    fprintf(stdout, "[%d] numbers [%d]: ", id, number_of_numbers);
    if (*numbers != NULL) {
        for (int i = 0; i < MIN(10, number_of_numbers); i++)
            fprintf(stdout, "%d ", (*numbers)[i]);
    }
    fprintf(stdout, "...\n");
#endif

        if (__error_code != EXIT_SUCCESS)
            return __error_code;
    }

    return EXIT_SUCCESS;
}