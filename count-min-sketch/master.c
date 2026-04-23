#include "util.h"
#include "master.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#include <mpi/mpi.h>

int _ReadInputFile(
    const int id,
    const char * input_file_path,
    int* numbers[] /* OUT */,
    int* number_of_numbers /* OUT */
);

int _NotifySlavesOfNumbersSize(
    const int id,
    const int number_of_numbers,
    const int world_size
);

int _PartitionDataToSlaves(
    const int id,
    const int* numbers,
    const int number_of_numbers,
    const int world_size,
    int ** receive_buffer /* OUT */,
    int* receive_number_of_numbers /* OUT */
);

int master(
    const int id,
    const char* input_file_path,
    const int world_size,
    const int number_of_hash_functions,
    const int number_of_counters
)
{
    int * numbers = NULL, number_of_numbers = 0;
    int * receive_buffer = NULL, receive_number_of_numbers = 0;
    count_min_sketch sketch;
    memset(&sketch, 0, sizeof(sketch));

    __try {
        int status = _ReadInputFile(id, input_file_path, &numbers, &number_of_numbers);
        if (status < 0) {
            fprintf(stderr, "[%d] Reading the input file failed!\n", id);
            __throw(MASTER_FAILED_ERROR_CODE);
        }

        status = count_min_sketch_create(number_of_counters, number_of_hash_functions, &sketch);
        if (status != 0) {
            fprintf(stderr, "[%d] Failed to create the global sketch!\n", id);
            __throw(MASTER_FAILED_ERROR_CODE);
        }

        // start of execution
        status = MPI_Barrier(MPI_COMM_WORLD);
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to set barrier!\n", id);
            __throw(status);
        }
        double t_start = MPI_Wtime();

        status = _NotifySlavesOfNumbersSize(id, number_of_numbers, world_size);
        if (status != 0) {
            fprintf(stderr, "[%d] Failed to notify slaves of the numbers size value!\n", id);
            __throw(MASTER_FAILED_ERROR_CODE);
        }

        status = _PartitionDataToSlaves(id, numbers, number_of_numbers, world_size, &receive_buffer, &receive_number_of_numbers);
        if (status != 0) {
            fprintf(stderr, "[%d] Failed to partition data to slaves!\n", id);
            __throw(MASTER_FAILED_ERROR_CODE);
        }

        status = do_task(id, receive_buffer, receive_number_of_numbers, number_of_counters, number_of_hash_functions, &sketch);
        if (status != 0) {
            fprintf(stderr,  "[%d] Failed to do task!\n", id);
            __throw(MASTER_FAILED_ERROR_CODE);
        }
        // end of execution
        status = MPI_Barrier(MPI_COMM_WORLD);
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to unset barrier!\n", id);
            __throw(status);
        }
        double t_end = MPI_Wtime();
        fprintf(stdout, "[%d] Execution time: %lf seconds\n", id, t_end - t_start);
    }
    __finally {
#ifdef CUSTOM_DEBUG_MASTER
    fprintf(stdout, "[%d] numbers [%d]: ", id, receive_number_of_numbers);
    if (receive_buffer != NULL) {
        for (int i = 0; i < MIN(10, receive_number_of_numbers); i++)
            fprintf(stdout, "%d ", receive_buffer[i]);
    }
    fprintf(stdout, "...\n");
#endif

        if (numbers != NULL)
            free(numbers), numbers = NULL, number_of_numbers = 0;
        if (receive_buffer != NULL)
            free(receive_buffer), receive_buffer = NULL, receive_number_of_numbers = 0;

        count_min_sketch_free(&sketch);
    }

    return __error_code;
}

int _ReadInputFile(
    const int id,
    const char * input_file_path,
    int* numbers[],
    int* number_of_numbers
)
{
    assert(numbers != NULL && *numbers == NULL);
    assert(number_of_numbers != NULL);
    assert(input_file_path != NULL);

    FILE * fd = NULL;

    __try {
        fd = fopen(input_file_path, "r");
        if (fd == NULL) {
            fprintf(stderr, "[%d] File at %s does not exist!", id, input_file_path);
            __throw(EXIT_FAILURE);
        }

        int bytes_read = fscanf(fd, "%d\n", number_of_numbers);
        if (bytes_read <= 0 || *number_of_numbers <= 0) {
            fprintf(stderr, "[%d] Number of numbers in the input file is smaller than 0!\n", id);
            __throw(EXIT_FAILURE);
        }

        *numbers = (int*)malloc(sizeof(int) * (*number_of_numbers));
        if (*numbers == NULL) {
            fprintf(stderr, "[%d] Failed to allocate numbers buffer!\n", id);
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < *number_of_numbers; ++i) {
            bytes_read = fscanf(fd, "%d\n", *numbers + i);
            if (bytes_read <= 0) {
                fprintf(stdout, "[%d] Failed to read number at index %d.\n", id, i);
            }
    }
    }
    __finally {
        if (fd != NULL && fclose(fd) < 0) {
            fprintf(stderr, "[%d] File descriptor failed to close!\n", id);
        }

        if (__error_code != EXIT_SUCCESS) {
            if (numbers != NULL)
                free(numbers), numbers = NULL, *number_of_numbers = 0;
            return __error_code;
        }
    }

    return EXIT_SUCCESS;
}

int _NotifySlavesOfNumbersSize(
    const int id,
    const int number_of_numbers,
    const int world_size
)
{
    int * numbers = NULL;
    __try {
        numbers = (int*)malloc(sizeof(int) * world_size);
        if (numbers == NULL) {
            fprintf(stderr, "[%d] Failed to allocate numbers * world_size buffer - numbers\n", id);
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < world_size; i++) {
            numbers[i] = (i < world_size - 1)
                ? number_of_numbers / world_size
                : number_of_numbers / world_size + number_of_numbers % world_size;
        }

        int recv_buf = 0;
        int status = MPI_Scatter(
            numbers,
            1,
            MPI_INT,
            &recv_buf,
            1,
            MPI_INT,
            0,
            MPI_COMM_WORLD
        );

        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to send number_of_numbers!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (__error_code != MPI_SUCCESS)
            return __error_code;

        if (numbers != NULL)
            free(numbers), numbers = NULL;
    }

    return EXIT_SUCCESS;
}

int _PartitionDataToSlaves(
    const int id,
    const int* numbers,
    const int number_of_numbers,
    const int world_size,
    int ** receive_buffer,
    int* receive_number_of_numbers
)
{
    assert(receive_buffer != NULL && *receive_buffer == NULL);
    assert(numbers != NULL);
    assert(world_size > 0);
    assert(number_of_numbers > 0);

    int * send_counts = NULL;
    int * offsets = NULL;
    __try {
        send_counts = (int*)malloc(sizeof(int) * world_size);
        if (send_counts == NULL) {
            fprintf(stderr, "[%d] Failed to allocate send_counts array!\n", id);
            __throw(EXIT_FAILURE);
        }

        offsets = (int*)malloc(sizeof(int) * world_size);
        if (offsets == NULL) {
            fprintf(stderr, "[%d] Failed to allocate offsets array!\n", id);
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < world_size; i++) {
            send_counts[i] = number_of_numbers / world_size;
            offsets[i] = i * (number_of_numbers / world_size);
        }
        send_counts[world_size - 1] += number_of_numbers % world_size; 

        *receive_number_of_numbers = send_counts[0];
        *receive_buffer = (int*)malloc(sizeof(int) * (*receive_number_of_numbers));
        if (*receive_buffer == NULL) {
            fprintf(stderr, "[%d] Failed to allocate the receive_buffer!\n", id);
            __throw(EXIT_FAILURE);
        }

        int status = MPI_Scatterv(
            numbers,
            send_counts,
            offsets,
            MPI_INT,
            *receive_buffer,
            *receive_number_of_numbers,
            MPI_INT,
            0,
            MPI_COMM_WORLD
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to scatter numbers!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (send_counts != NULL)
            free(send_counts), send_counts = NULL;

        if (offsets != NULL)
            free(offsets), offsets = NULL;

        if (__error_code != EXIT_SUCCESS) {
            if (receive_buffer != NULL)
                free(receive_buffer), receive_buffer = NULL, receive_number_of_numbers = 0;
            return __error_code;
        }
    }

    return EXIT_SUCCESS;
}