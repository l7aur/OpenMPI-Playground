#include "util.h"

#include <stdio.h>
#include <assert.h>

int _CannonTickUp(
    const int id,
    mat* matrix,
    const int tag,
    MPI_Comm* cartesianComm
);

int _CannonTickLeft(
    const int id,
    mat* matrix,
    const int tag,
    MPI_Comm* cartesianComm
);

int _CannonTick(
    const int id,
    mat* matrix,
    const int tag,
    Direction d,
    MPI_Comm* cartesianComm
);

int Parse(
    const int argc,
    const char* argv[],
    const unsigned int gridLength,
    mat** matrix_a,
    mat** matrix_b
) {
    assert(matrix_a != NULL);
    assert(matrix_b != NULL);

    if (argc != 3) {
        fprintf(stderr, "Usage ./main <path_to_matrix_a> <path_to_matrix_b>\n");
        return EXIT_FAILURE;
    }

    __try {
        *matrix_a = MatrixRead(gridLength, argv[1]); 
        if (*matrix_a == NULL) {
            fprintf(stderr, "Failed to read matrix_a.\n");
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        MatrixPrint(*matrix_a, stdout);
        printf("\n");
#endif

        *matrix_b = MatrixRead(gridLength, argv[2]);
        if (*matrix_b == NULL) {
            fprintf(stderr, "Failed to read matrix_b.\n");
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        MatrixPrint(*matrix_b, stdout);
#endif
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (*matrix_a != NULL)
                MatrixDeallocate(matrix_a);

            if (*matrix_b != NULL)
                MatrixDeallocate(matrix_b);

            return EXIT_FAILURE;    
        }
    }
    return EXIT_SUCCESS;
}

int CanonAlgorithm(
    const int id,
    mat* a, 
    mat* b,
    const int numberOfIterations,
    MPI_Comm* cartesianComm,
    mat** c
) {
    assert(c != NULL);
    
    __try {
        *c = MatrixAllocate(a->rows, b->cols);
        if (c == NULL) {
            fprintf(stderr, "Failed to allocate result matrix!\n");
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < numberOfIterations; i++) {
            MatrixElementwiseMultiply(a, b, *c);
            _CannonTick(id, a, MATRIX_A_TAG, Left, cartesianComm);
            _CannonTick(id, b, MATRIX_B_TAG, Up, cartesianComm);
        }        
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (*c != NULL)
                MatrixDeallocate(c);

            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int _CannonTick(
    const int id,
    mat* matrix,
    const int tag,
    Direction d,
    MPI_Comm* cartesianComm
) {
    assert(d == Left || d == Up);

    if (d == Left)
        return _CannonTickLeft(id, matrix, tag, cartesianComm);
    if (d == Up)
        return _CannonTickUp(id, matrix, tag, cartesianComm);
    
    return EXIT_FAILURE;
}

int _CannonTickUp(
    const int id,
    mat *matrix,
    const int tag,
    MPI_Comm* cartesianComm
)
{
    int sourceRank = 0, destRank = 0;

    int status = MPI_Cart_shift(
        *cartesianComm,
        0,
        -1,
        &sourceRank,
        &destRank
    );
    if (status != MPI_SUCCESS) {
        fprintf(stderr, "[%d] Failed to find neighbors!\n", id);
        return EXIT_FAILURE;
    }

    status = MPI_Sendrecv_replace(
        matrix->data,
        matrix->rows * matrix->cols,
        MPI_INT,
        destRank,
        tag,
        sourceRank,
        tag,
        *cartesianComm,
        MPI_STATUS_IGNORE
    );
    if (status != MPI_SUCCESS) {
        fprintf(stderr, "[%d] Failed to tick!\n", id);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int _CannonTickLeft(
    const int id,
    mat *matrix,
    const int tag,
    MPI_Comm* cartesianComm
)
{
    int sourceRank = 0, destRank = 0;

    int status = MPI_Cart_shift(
        *cartesianComm,
        1,
        -1,
        &sourceRank,
        &destRank
    );
    if (status != MPI_SUCCESS) {
        fprintf(stderr, "[%d] Failed to find neighbors!\n", id);
        return EXIT_FAILURE;
    }

    status = MPI_Sendrecv_replace(
        matrix->data,
        matrix->rows * matrix->cols,
        MPI_INT,
        destRank,
        tag,
        sourceRank,
        tag,
        *cartesianComm,
        MPI_STATUS_IGNORE
    );
    if (status != MPI_SUCCESS) {
        fprintf(stderr, "[%d] Failed to tick!\n", id);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int CollectResult(

) {

    return EXIT_SUCCESS;
}