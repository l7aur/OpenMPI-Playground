#include "util.h"
#include "slave.h"

mat* _SlaveReceiveSetupMatrix(
    const int id,
    const int coords[WORLD_DIMENSIONS],
    MPI_Comm * cartesianComm,
    const int tag
);

int Slave(
    const int id,
    const int coords[WORLD_DIMENSIONS],
    MPI_Comm* cartesianComm
) {

    mat* my_a = NULL;
    mat* my_b = NULL;
    __try {
        my_a = _SlaveReceiveSetupMatrix(id, coords, cartesianComm, MATRIX_A_TAG);
        if (my_a == NULL) {
            fprintf(stderr, "[%d] Failed to receive a setup matrix (%d; %d)!\n", id, coords[0], coords[1]);
            __throw(EXIT_FAILURE);
        }

        my_b = _SlaveReceiveSetupMatrix(id, coords, cartesianComm, MATRIX_B_TAG);
        if (my_b == NULL) {
            fprintf(stderr, "[%d] Failed to receive b setup matrix (%d; %d)!\n", id, coords[0], coords[1]);
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        printf("[%d] (%d; %d) My setup matrix A is:\n", id, coords[0], coords[1]);
        MatrixPrint(my_a, stdout);
        printf("[%d] (%d; %d) My setup matrix B is:\n", id, coords[0], coords[1]);
        MatrixPrint(my_b, stdout);
#endif
    }
    __finally {
        if (my_a != NULL)
            MatrixDeallocate(&my_a);
        if (my_b != NULL)
            MatrixDeallocate(&my_b);

        if (__error_code != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

mat* _SlaveReceiveSetupMatrix(
    const int id,
    const int coords[WORLD_DIMENSIONS],
    MPI_Comm * cartesianComm,
    const int tag
) {
    mat * matrix = NULL;
    __try {
        unsigned int matrixSize[WORLD_DIMENSIONS] = { 0, 0 };
        int status = MPI_Recv(
            matrixSize,
            WORLD_DIMENSIONS,
            MPI_UNSIGNED,
            0,
            tag,
            *cartesianComm,
            MPI_STATUS_IGNORE
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to retrieve matrix setup sizes (%d; %d)!\n", id, coords[0], coords[1]);
            __throw(EXIT_FAILURE);
        }

        matrix = MatrixAllocate(matrixSize[0], matrixSize[1]);
        if (matrix == NULL) {
            fprintf(stderr, "[%d] Failed to allocate matrix (%d; %d)!\n", id, coords[0], coords[1]);
            __throw(EXIT_FAILURE);
        }

        status = MPI_Recv(
            matrix->data,
            matrix->cols * matrix->rows,
            MPI_INT,
            0,
            tag,
            *cartesianComm,
            MPI_STATUS_IGNORE
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "[%d] Failed to retrieve matrix setup data (%d; %d)!\n", id, coords[0], coords[1]);
            __throw(EXIT_FAILURE);
        }

    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (matrix != NULL)
                MatrixDeallocate(&matrix);

            return NULL;
        }
    }

    return matrix;
}