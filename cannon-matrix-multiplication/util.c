#include "util.h"

#include <stdio.h>
#include <assert.h>

int Parse(
    const int argc,
    const char* argv[],
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
        *matrix_a = MatrixRead(argv[1]); 
        if (*matrix_a == NULL) {
            fprintf(stderr, "Failed to read matrix_a.\n");
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        MatrixPrint(*matrix_a, stdout);
        printf("\n");
#endif

        *matrix_b = MatrixRead(argv[2]);
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
                MatrixDeallocate(*matrix_a);

            if (*matrix_b != NULL)
                MatrixDeallocate(*matrix_b);

            return EXIT_FAILURE;    
        }
    }
    return EXIT_SUCCESS;
}