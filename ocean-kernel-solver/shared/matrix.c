#include "util.h"
#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

matrix* MatrixAllocate(
    const unsigned int rows,
    const unsigned int cols
) {
    assert(rows > 0); assert(cols > 0);

    matrix* m = (matrix*)malloc(sizeof(matrix) * 1);
    if (m == NULL) {
        fprintf(stderr, "Failed to allocate matrix!\n");
        return m;
    }
    m->rows = rows;
    m->cols = cols;
    __try {
        m->data = (MATRIX_NNNER_DATA_TYPE*)malloc(sizeof(MATRIX_NNNER_DATA_TYPE) * m->cols * m->rows);
        if (m->data == NULL) {
            fprintf(stderr, "Failed to allocate data buffer for matrix!\n");
            __throw(EXIT_FAILURE);
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS && m != NULL) {
            free(m), m = NULL;
        }
    }
    return m;
}

void MatrixDeallocate(
    matrix* m
) {
    assert(m != NULL);

    if (m->data != NULL)
        free(m->data), m->data = NULL;

    free(m), m = NULL;
}

MATRIX_NNNER_DATA_TYPE* MatrixAddressAt(
    const matrix* m,
    const unsigned int row,
    const unsigned int col
) {
    assert(row > 0); assert(col > 0);
    assert(row < m->rows); assert(col < m->cols);
    assert(m->data != NULL);

    return &(m->data[row * m->cols + col]);
}

void MatrixSetPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
) {
    assert(m != NULL); assert(m->data != NULL);
    assert(border_padding > 0);

    for (unsigned int r = 0; r < m->rows; r++)
        for (unsigned int padding = 0; padding < border_padding; padding++) {
            *MatrixAddressAt(m, r + padding, 0) = padding_value;
            *MatrixAddressAt(m, m->rows - 1 - padding, 0) = padding_value;
        }

    for (unsigned int c = 0; c < m->cols; c++)
        for (unsigned int padding = 0; padding < border_padding; padding++) {
            *MatrixAddressAt(m, 0, c + padding) = padding_value;
            *MatrixAddressAt(m, 0, m->cols - 1 - padding) = padding_value;
        }
}