#include "util.h"
#include "master.h"

#include <assert.h>
#include <memory.h>

int _MasterSetGrid(
    const int id,
    const unsigned int gridLength,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm,
    mat** my_a /* OUT */,
    mat** my_b /* OUT */
);

void _MasterRearrangePartitionsShiftLeft(
    mat** partition,
    const unsigned int gridLength
);

void _MasterRearrangePartitionsShiftUp(
    mat** partition,
    const unsigned int gridLength
);

int _MasterSendMatrix(
    const mat* matrix,
    const int targetCoords[WORLD_DIMENSIONS],
    const MPI_Comm* cartesianComm,
    const int tag
);

void _MasterFreePartitions(
    mat*** partitions,
    const unsigned int numberOfPartitions
);

int _MasterCollectResults(
    const int id,
    const unsigned int gridLength,
    mat** my_res,
    mat** result,
    MPI_Comm* cartesianComm
);

int Master(
    const int id,
    const unsigned int gridLength,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm
) {
    assert(matrix_a != NULL && matrix_b != NULL);
    assert(coords != NULL);
    assert(cartesianComm != NULL);
    assert(gridLength > 0);
    
    mat * my_a = NULL;
    mat * my_b = NULL;
    mat * my_c = NULL;
    mat* result = NULL;
    __try {
        if (_MasterSetGrid(
            id,
            gridLength,
            coords, 
            matrix_a, 
            matrix_b, 
            cartesianComm, 
            &my_a, 
            &my_b
        ) != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Setting the grid up failed!\n", id);
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        printf("[%d] (%d; %d) My setup matrix A is:\n", id, coords[0], coords[1]);
        MatrixPrint(my_a, stdout);
        printf("[%d] (%d; %d) My setup matrix B is:\n", id, coords[0], coords[1]);
        MatrixPrint(my_b, stdout);
#endif

        if (CanonAlgorithm(
            id,
            my_a,
            my_b,
            gridLength,
            cartesianComm,
            &my_c
        ) != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Failed to execute Cannon's algorithm!\n", id);
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG_ALG
        printf("[%d] (%d; %d) My result matrix C is:\n", id, coords[0], coords[1]);
        MatrixPrint(my_c, stdout);
        printf("\n");
#endif

        if (_MasterCollectResults(
            id,
            gridLength,
            &my_c,
            &result,
            cartesianComm
        ) != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Failed to call result collection!\n", id);
            __throw(EXIT_FAILURE);
        }

#ifdef PRINT_RESULT
        printf("Result of computation:\n");
        MatrixPrint(result, stdout);
#endif

    }
    __finally {
        if (my_a != NULL)
            MatrixDeallocate(&my_a);
        if (my_b != NULL)
            MatrixDeallocate(&my_b);
        if (my_c != NULL)
            MatrixDeallocate(&my_c);
        if (result != NULL)
            MatrixDeallocate(&result);

        if (__error_code != EXIT_SUCCESS) {
            return __error_code;
        }
    }
    return EXIT_SUCCESS;
}

int _MasterSetGrid(
    const int id,
    const unsigned int gridLength,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm,
    mat** my_a,
    mat** my_b
) {
    assert(matrix_a != NULL && matrix_b != NULL);
    assert(coords != NULL);
    assert(cartesianComm != NULL);
    assert(my_a != NULL && my_b != NULL);

    mat** partitions_a = NULL;
    mat** partitions_b = NULL;
    
    __try {
        int status = MatrixPartition(gridLength, matrix_a, &partitions_a);
        if (status != EXIT_SUCCESS || partitions_a == NULL) {
            fprintf(stderr, "[%d] Failed to partition matrix a!\n", id);
            __throw(status);
        }

        status = MatrixPartition(gridLength, matrix_b, &partitions_b);
        if (status != EXIT_SUCCESS || partitions_b == NULL) {
            fprintf(stderr, "[%d] Failed to partition matrix b!\n", id);
            __throw(status);
        }

        _MasterRearrangePartitionsShiftLeft(partitions_a, gridLength);
        _MasterRearrangePartitionsShiftUp(partitions_b, gridLength);

        *my_a = partitions_a[0];
        partitions_a[0] = NULL;
        
        *my_b = partitions_b[0];
        partitions_b[0] = NULL;
        
        // todo convert to MPI_Scatter
        for (int i = 0; i < gridLength; i++)
            for (int j = 0; j < gridLength; j++) {
                if (i == 0 && j == 0)
                    continue;

                int targetCoords[WORLD_DIMENSIONS] = { i, j };

                mat* currentMatrix_a = partitions_a[i * gridLength + j];
                int status = _MasterSendMatrix(
                    currentMatrix_a,
                    targetCoords,
                    cartesianComm,
                    MATRIX_A_TAG
                );
                if (status != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to send setup matrix a of process (%d; %d)!\n", i, j);
                    __throw(EXIT_FAILURE);
                }
                MatrixDeallocate(&partitions_a[i * gridLength + j]);

                mat* currentMatrix_b = partitions_b[i * gridLength + j];
                status = _MasterSendMatrix(
                    currentMatrix_b,
                    targetCoords,
                    cartesianComm,
                    MATRIX_B_TAG
                );
                if (status != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to send setup matrix b of process (%d; %d)!\n", i, j);
                    __throw(EXIT_FAILURE);
                }
                MatrixDeallocate(&partitions_b[i * gridLength + j]);
            }
    }
    __finally {
        _MasterFreePartitions(&partitions_a, gridLength * gridLength);            
        _MasterFreePartitions(&partitions_b, gridLength * gridLength);

        if (__error_code != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void _MasterRearrangePartitionsShiftLeft(
    mat** partition,
    const unsigned int gridLength
) {
    assert(gridLength > 0);
    assert(partition != NULL);

    for (int i = 1; i < gridLength; i++) {
        // Shift Row i left by i positions
        for (int shift = 0; shift < i; shift++) {
            mat* first = partition[i * gridLength];
            for (int j = 0; j < gridLength - 1; j++) {
                partition[i * gridLength + j] = partition[i * gridLength + j + 1];
            }
            partition[i * gridLength + gridLength - 1] = first;
        }
    }
}

void _MasterRearrangePartitionsShiftUp(
    mat** partition,
    const unsigned int gridLength
) {
    assert(gridLength > 0);
    assert(partition != NULL);

    for (int j = 1; j < gridLength; j++) {
        // Shift Column j up by j positions
        for (int shift = 0; shift < j; shift++) {
            mat* top = partition[j];
            for (int i = 0; i < gridLength - 1; i++) {
                partition[i * gridLength + j] = partition[(i + 1) * gridLength + j];
            }
            partition[(gridLength - 1) * gridLength + j] = top;
        }
    }
}

int _MasterSendMatrix(
    const mat* matrix,
    const int targetCoords[WORLD_DIMENSIONS],
    const MPI_Comm* cartesianComm,
    const int tag
) {
    assert(matrix != NULL);
    assert(targetCoords != NULL && targetCoords[0] >= 0 && targetCoords[1] >= 0);
    assert(!(targetCoords[0] == 0 && targetCoords[1] == 0));
    assert(cartesianComm != NULL);

    __try {
        int targetRank = -1;

        int status = MPI_Cart_rank(
            *cartesianComm,
            targetCoords,
            &targetRank
        );
        if (status != MPI_SUCCESS || targetRank < 0) {
            fprintf(stderr, "Failed to retrieve rank of process (%d; %d)!\n", targetCoords[0], targetCoords[1]);
            __throw(EXIT_FAILURE);
        }

        unsigned int matrixDimensions[WORLD_DIMENSIONS] = { matrix->rows, matrix->cols };
        status = MPI_Send(
            matrixDimensions,
            WORLD_DIMENSIONS,
            MPI_UNSIGNED,
            targetRank,
            tag,
            *cartesianComm
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "Failed to send buffer size to process (%d; %d)!\n", targetCoords[0], targetCoords[1]);
            __throw(EXIT_FAILURE);
        }

        status = MPI_Send(
            matrix->data,
            matrix->rows * matrix->cols,
            MPI_INT,
            targetRank,
            tag,
            *cartesianComm
        );
        if (status != MPI_SUCCESS) {
            fprintf(stderr, "Failed to send setup data to process (%d; %d)!\n", targetCoords[0], targetCoords[1]);
            __throw(EXIT_FAILURE);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void _MasterFreePartitions(
    mat*** partitions,
    const unsigned int numberOfPartitions
)
{
    assert(partitions != NULL);

    for (int i = 0; i < numberOfPartitions; ++i) {
        if ((*partitions)[i] == NULL)
            continue;

        MatrixDeallocate(partitions[i]);
    }
    free(*partitions), *partitions = NULL;
}

int _MasterCollectResults(
    const int id,
    const unsigned int gridLength,
    mat** my_res,
    mat** result,
    MPI_Comm* cartesianComm
) {
    assert(result != NULL && *result == NULL);
    assert(my_res != NULL);
    assert(cartesianComm != NULL);
    assert(gridLength > 0);

    int * auxBuffer = NULL;
    __try {

        auxBuffer = (int*)malloc(sizeof(int) * (*my_res)->rows * (*my_res)->cols);
        if (auxBuffer == NULL) {
            fprintf(stderr, "[%d] Failed to allocate aux buffer for result!\n", id);
            __throw(EXIT_FAILURE);    
        }

        *result = MatrixAllocate((*my_res)->rows * gridLength, (*my_res)->cols * gridLength);
        if (*result == NULL) {
            fprintf(stderr, "[%d] Failed to allocate result buffer!\n", id);
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < gridLength; ++i)
            for (int j = 0; j < gridLength; ++j) {
                if (i == 0 && j == 0) {
                    MatrixFill(
                        *result,
                        i * (*my_res)->rows,
                        j * (*my_res)->cols,
                        (*my_res)->data,
                        (*my_res)->rows,
                        (*my_res)->cols
                    );
                    continue;
                }
                
                int coords[WORLD_DIMENSIONS] = { i, j };
                int source = -1;
                int status = MPI_Cart_rank(*cartesianComm, coords, &source);
                if (status != MPI_SUCCESS) {
                    fprintf(stderr, "[%d] Failed to retrieve rank for (%d, %d)!\n", id, i, j);
                    __throw(EXIT_FAILURE);
                }

                status = MPI_Recv(
                    auxBuffer,
                    (*my_res)->rows * (*my_res)->cols,
                    MPI_INT,
                    source,
                    MATRIX_C_TAG,
                    *cartesianComm,
                    MPI_STATUS_IGNORE
                );
                if (status != MPI_SUCCESS) {
                    fprintf(stderr, "[%d] Failed to retrieve data from (%d; %d)!\n", id, coords[0], coords[1]);
                    return EXIT_FAILURE;
                }
                
                MatrixFill(
                    *result,
                    i * (*my_res)->rows,
                    j * (*my_res)->cols,
                    auxBuffer,
                    (*my_res)->rows,
                    (*my_res)->cols
                );
            }
    }
    __finally {
        if (auxBuffer != NULL)
            free(auxBuffer);

        if (__error_code != EXIT_SUCCESS) {
            if (*result != NULL)
                MatrixDeallocate(result);
            return EXIT_FAILURE;
        }
    }

    MatrixDeallocate(my_res);

    return EXIT_SUCCESS;
}