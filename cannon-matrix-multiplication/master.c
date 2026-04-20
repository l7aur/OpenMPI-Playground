#include "util.h"
#include "master.h"

#include <assert.h>
#include <memory.h>

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
    mat** matrix_b
) {
    assert(matrix_a != NULL && matrix_b != NULL);
    assert(coords != NULL);

    mat** partitions_a = NULL;
    mat** partitions_b = NULL;
    
    __try {
        int status = MatrixPartition(gridWidth, gridHeight, matrix_a, &partitions_a);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Failed to partition matrix a!\n", id);
            __throw(status);
        }

        status = MatrixPartition(gridWidth, gridHeight, matrix_b, &partitions_b);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "[%d] Failed to partition matrix b!\n", id);
            __throw(status);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            _MasterFreePartitions(&partitions_a, gridWidth * gridHeight);            
            _MasterFreePartitions(&partitions_b, gridWidth * gridHeight);
        }
    }
    return EXIT_SUCCESS;
}

void _MasterFreePartitions(
    mat*** partitions,
    const unsigned int numberOfPartitions
)
{
    for (int m = 0; m < MAX_NUMBER_OF_MATRICES; m++) {
        if (partitions[m] == NULL) 
            continue;

        for (int i = 0; i < numberOfPartitions; ++i) {
            if (partitions[m][i] == NULL)
                continue;

            MatrixDeallocate(&partitions[m][i]);
        }
        free(partitions[m]), partitions[m] = NULL;
    }
}