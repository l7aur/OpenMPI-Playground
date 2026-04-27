#include "util.h"
#include "matrix.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#define _USE_MATH_DEFINES
#include <math.h>

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

void MatrixSetUpPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
) {
    assert(m != NULL); assert(m->data != NULL);
    assert(border_padding > 0);

    for (unsigned int c = 0; c < m->cols; c++)
        for (unsigned int padding = 0; padding < border_padding; padding++)
            *MatrixAddressAt(m, padding, c) = padding_value;
}

void MatrixSetDownPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
) {
    assert(m != NULL); assert(m->data != NULL);
    assert(border_padding > 0);

    for (unsigned int c = 0; c < m->cols; c++)
        for (unsigned int padding = 0; padding < border_padding; padding++)
            *MatrixAddressAt(m, m->rows - 1 - padding, c) = padding_value;
}

void MatrixSetLeftPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
) {
    assert(m != NULL); assert(m->data != NULL);
    assert(border_padding > 0);

    for (unsigned int r = 0; r < m->rows; r++)
        for (unsigned int padding = 0; padding < border_padding; padding++)
            *MatrixAddressAt(m, r, padding) = padding_value;
}

void MatrixSetRightPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
) {
    assert(m != NULL); assert(m->data != NULL);
    assert(border_padding > 0);

    for (unsigned int r = 0; r < m->rows; r++)
        for (unsigned int padding = 0; padding < border_padding; padding++)
            *MatrixAddressAt(m, r, m->cols - 1 - padding) = padding_value;
}

void MatrixInit(
    matrix* m,
    const unsigned int level_r,
    const unsigned int level_c,
    const unsigned int level_number_of_elements,
    const unsigned int border_padding
) {
    double factor = M_PI / (double)level_number_of_elements;
    for (unsigned int r = border_padding; r < m->rows - border_padding; r++)
        for (unsigned int c = border_padding; c < m->cols - border_padding; c++)
            *MatrixAddressAt(m, r, c) = sinl((level_r + r) * factor) * sinl((level_c + c) * factor);
}