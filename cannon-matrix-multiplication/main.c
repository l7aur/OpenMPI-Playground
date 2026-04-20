#include "util.h"
#include "slave.h"
#include "master.h"

int main(const int argc, const char* argv[])
{
    __try {
        mat* matrix_a = NULL;
        mat* matrix_b = NULL;
        if (Parse(argc, argv, &matrix_a, &matrix_b) != EXIT_SUCCESS || matrix_a == NULL || matrix_b == NULL) {
            printf("Parsing failed: matrix_a = %p, matrix_b = %p!\n", matrix_a, matrix_b);
            __throw(EXIT_FAILURE);
        }

        int gridWidth = 3, gridHeight = 3;
        mat** partitions = NULL;
        int status = MatrixPartition(gridWidth, gridHeight, &matrix_a, &partitions);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to partition input matrix!\n");
            __throw(status);
        }

        for (int i = 0; i < gridWidth * gridHeight; i++) {
            MatrixPrint(partitions[i], stdout);
            printf("\n");
        }

        if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
            printf("Failed to initialize MPI.\n");
            __throw(EXIT_FAILURE);
        }

        

    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            
            int flag = 0;
            if (MPI_Initialized(&flag) == MPI_SUCCESS && flag) {
                MPI_Abort(MPI_COMM_WORLD, __error_code);
            }
            return __error_code;
        }
    }

    int status = MPI_Finalize();
    if (status < 0) 
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}