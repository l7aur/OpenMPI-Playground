#include "util.h"
#include "master.h"

#include <assert.h>
#include <memory.h>

int _MasterSetGrid(
    const int id,
    const unsigned int gridWidth,
    const unsigned int gridHeight,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm,
    mat** my_a /* OUT */,
    mat** my_b /* OUT */
);

void _MasterRearrangePartitionsShiftLeft(
    mat** partition,
    const unsigned int gridWidth,
    const unsigned int gridHeight
);

void _MasterRearrangePartitionsShiftUp(
    mat** partition,
    const unsigned int gridWidth,
    const unsigned int gridHeight
);

int _MasterSendSetupMatrix(
    const mat* matrix,
    const int targetCoords[WORLD_DIMENSIONS],
    const MPI_Comm* cartesianComm,
    const int tag
);

void _MasterFreePartitions(
    mat*** partitions,
    const unsigned int numberOfPartitions
);

int Master(
    const int id,
    const unsigned int gridWidth,
    const unsigned int gridHeight,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm
) {
    assert(matrix_a != NULL && matrix_b != NULL);
    assert(coords != NULL);
    assert(cartesianComm != NULL);
    
    mat * my_a = NULL, * my_b = NULL;
    __try {
        int status = _MasterSetGrid(id, gridWidth, gridHeight, coords, matrix_a, matrix_b, cartesianComm, &my_a, &my_b);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Setting the grid up failed!\n", id);
            __throw(status);
        }
        
    }
    __finally {
        if (my_a != NULL)
            MatrixDeallocate(&my_a);

        if (my_b != NULL)
            MatrixDeallocate(&my_b);

        if (__error_code != EXIT_SUCCESS) {
            return __error_code;
        }
    }
    return EXIT_SUCCESS;
}

int _MasterSetGrid(
    const int id,
    const unsigned int gridWidth,
    const unsigned int gridHeight,
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
        int status = MatrixPartition(gridWidth, gridHeight, matrix_a, &partitions_a);
        if (status != EXIT_SUCCESS || partitions_a == NULL) {
            fprintf(stderr, "[%d] Failed to partition matrix a!\n", id);
            __throw(status);
        }

        status = MatrixPartition(gridWidth, gridHeight, matrix_b, &partitions_b);
        if (status != EXIT_SUCCESS || partitions_b == NULL) {
            fprintf(stderr, "[%d] Failed to partition matrix b!\n", id);
            __throw(status);
        }

        _MasterRearrangePartitionsShiftLeft(partitions_a, gridWidth, gridHeight);
        _MasterRearrangePartitionsShiftUp(partitions_b, gridWidth, gridHeight);

        *my_a = partitions_a[0];
        partitions_a[0] = NULL;
        
        *my_b = partitions_b[0];
        partitions_b[0] = NULL;

        for (int i = 0; i < gridHeight; i++)
            for (int j = 0; j < gridWidth; j++) {
                if (i == 0 && j == 0)
                    continue;

                int targetCoords[WORLD_DIMENSIONS] = { i, j };

                mat* currentMatrix_a = partitions_a[i * gridWidth + j];
                int status = _MasterSendSetupMatrix(
                    currentMatrix_a,
                    targetCoords,
                    cartesianComm,
                    MATRIX_A_TAG
                );
                if (status != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to send setup matrix a of process (%d; %d)!\n", i, j);
                    __throw(EXIT_FAILURE);
                }
                MatrixDeallocate(&partitions_a[i * gridWidth + j]);

                mat* currentMatrix_b = partitions_b[i * gridWidth + j];
                status = _MasterSendSetupMatrix(
                    currentMatrix_b,
                    targetCoords,
                    cartesianComm,
                    MATRIX_B_TAG
                );
                if (status != EXIT_SUCCESS) {
                    fprintf(stderr, "Failed to send setup matrix b of process (%d; %d)!\n", i, j);
                    __throw(EXIT_FAILURE);
                }
                MatrixDeallocate(&partitions_b[i * gridWidth + j]);
            }
    }
    __finally {
        _MasterFreePartitions(&partitions_a, gridWidth * gridHeight);            
        _MasterFreePartitions(&partitions_b, gridWidth * gridHeight);

        if (__error_code != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void _MasterRearrangePartitionsShiftLeft(
    mat** partition,
    const unsigned int gridWidth,
    const unsigned int gridHeight
) {
    assert(gridWidth > 0 && gridHeight > 0);
    assert(partition != NULL);

    for (int i = 1; i < gridHeight; i++) {
        // Shift Row i left by i positions
        for (int shift = 0; shift < i; shift++) {
            mat* first = partition[i * gridWidth];
            for (int j = 0; j < gridWidth - 1; j++) {
                partition[i * gridWidth + j] = partition[i * gridWidth + j + 1];
            }
            partition[i * gridWidth + gridWidth - 1] = first;
        }
    }
}

void _MasterRearrangePartitionsShiftUp(
    mat** partition,
    const unsigned int gridWidth,
    const unsigned int gridHeight
) {
    assert(gridWidth > 0 && gridHeight > 0);
    assert(partition != NULL);

    for (int j = 1; j < gridWidth; j++) {
        // Shift Column j up by j positions
        for (int shift = 0; shift < j; shift++) {
            mat* top = partition[j];
            for (int i = 0; i < gridHeight - 1; i++) {
                partition[i * gridWidth + j] = partition[(i + 1) * gridWidth + j];
            }
            partition[(gridHeight - 1) * gridWidth + j] = top;
        }
    }
}

int _MasterSendSetupMatrix(
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