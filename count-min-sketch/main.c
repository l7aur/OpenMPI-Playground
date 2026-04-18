#include "util.h"
#include "slave.h"
#include "master.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <mpi/mpi.h>

int main(int argc, char* argv[])
{
    char input_file_path[INPUT_FILE_PATH_MAX_SIZE];
    memset(&input_file_path, 0, sizeof(char) * INPUT_FILE_PATH_MAX_SIZE);    
    int number_of_hash_functions = 0;
    int number_of_counters = 0;

    int status = parse(
        argc,
        argv,
        input_file_path,
        &number_of_hash_functions,
        &number_of_counters
    );
    if (status < 0) {
        fprintf(stderr, "Parsing command line arguments failed!\n");
        return EXIT_FAILURE;
    }

    assert(input_file_path != NULL);
    assert(number_of_hash_functions >=3 && number_of_hash_functions <= 5);
    assert(number_of_counters > 0);

    MPI_Init(NULL, NULL);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 1) {
        fprintf(stderr, "World size must be >= 1 for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, INITIALIZATION_FAILED_ERROR_CODE);
    }

    if (world_rank == 0) {
        int status = master(
            world_rank,
            input_file_path,
            world_size,
            number_of_hash_functions,
            number_of_counters
        );
        if (status < 0)
            MPI_Abort(MPI_COMM_WORLD, MASTER_FAILED_ERROR_CODE);
    }
    else {
        int status = slave(
            world_rank,
            number_of_hash_functions,
            number_of_counters    
        );
        if (status < 0)
            MPI_Abort(MPI_COMM_WORLD, SLAVE_FAILED_ERROR_CODE);
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}