#include "util.h"
#include "slave.h"
#include "master.h"

#include <memory.h>

int main(const int argc, const char* argv[])
{
    mat* matrix_a = NULL;
    mat* matrix_b = NULL;

    __try {
        if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
            printf("Failed to initialize MPI.\n");
            __throw(EXIT_FAILURE);
        }

        int size = 0;
        if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS) {
            fprintf(stderr, "Failed to retrieve size of communicator!\n");
            __throw(EXIT_FAILURE);
        }

        int dims[WORLD_DIMENSIONS] = { 0, 0 };
        if (MPI_Dims_create(size, WORLD_DIMENSIONS, dims) != MPI_SUCCESS){
            fprintf(stderr, "Failed to initialize world dimensions!\n");
            __throw(EXIT_FAILURE);
        }

        int periods[WORLD_DIMENSIONS] = { 1, 1 };
        MPI_Comm cartesianComm;
        if (MPI_Cart_create(
            MPI_COMM_WORLD,
            WORLD_DIMENSIONS,
            dims,
            periods,
            MPI_ALLOW_REORDER,
            &cartesianComm) != MPI_SUCCESS) {
            fprintf(stderr, "Failed to create cartesian communicator!\n");
            __throw(EXIT_FAILURE);
        }

        int rank = 0;
        if (MPI_Comm_rank(cartesianComm, &rank) != MPI_SUCCESS) {
            fprintf(stderr, "Failed to retrieve rank of process!\n");
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        if (rank == 0) {
            printf("[INFO]: World dimension is %dx%d.\n", dims[0], dims[1]);
        }
#endif

        int coords[WORLD_DIMENSIONS];
        memset(coords, 0, sizeof(coords));
        if (MPI_Cart_coords(cartesianComm, rank, WORLD_DIMENSIONS, coords) != MPI_SUCCESS) {
            fprintf(stderr, "Failed to retrieve process [%d] coordinates in grid!\n", rank);
            __throw(EXIT_FAILURE);
        }

        int status = EXIT_SUCCESS;
        if (rank == 0) {
            if (GRID_DIMENSION(size) * GRID_DIMENSION(size) != size) {
                fprintf(stderr, "The number of processes must be a perfect square!\n");
                __throw(EXIT_FAILURE);
            }
            
            if (Parse(argc, argv, &matrix_a, &matrix_b) != EXIT_SUCCESS || matrix_a == NULL || matrix_b == NULL) {
                printf("Parsing failed: matrix_a = %p, matrix_b = %p!\n", matrix_a, matrix_b);
                __throw(EXIT_FAILURE);
            }

            if (matrix_a->cols != matrix_b->rows) {
                fprintf(stderr, "%dx%d matrix cannot be multiplied by %dx%d matix!\nRevise input!\n",
                    matrix_a->rows, matrix_a->cols, matrix_b->rows, matrix_b->cols);
                __throw(EXIT_FAILURE);
            }

            status = Master(
                rank,
                dims[1],
                dims[0],
                coords,
                &matrix_a,
                &matrix_b,
                &cartesianComm
            );
        }
        else {
            status = Slave(
                rank,
                coords,
                &cartesianComm
            );
        }

        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "Worker at (%d; %d) failed!\n", coords[0], coords[1]);
            __throw(status);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            int flag = 0;
            if (MPI_Initialized(&flag) != MPI_SUCCESS) {
                fprintf(stderr, "MPI status is unknown!\n");
            }
            if (flag) {
                MPI_Abort(MPI_COMM_WORLD, __error_code);
            }
            return __error_code;
        }

        if (matrix_a != NULL)
            free(matrix_a), matrix_a = NULL;

        if (matrix_b != NULL)
            free(matrix_b), matrix_b = NULL;
    }

    if (MPI_Finalize() != MPI_SUCCESS) 
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}