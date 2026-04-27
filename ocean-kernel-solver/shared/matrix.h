#pragma once

#define MATRIX_NNNER_DATA_TYPE double

typedef struct _matrix {
    unsigned int rows;
    unsigned int cols;
    MATRIX_NNNER_DATA_TYPE* data;
} matrix;

matrix* MatrixAllocate(
    const unsigned int rows,
    const unsigned int cols
);

void MatrixDeallocate(
    matrix* m
);

MATRIX_NNNER_DATA_TYPE* MatrixAddressAt(
    const matrix* m,
    const unsigned int row,
    const unsigned int col
);

void MatrixSetUpPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
);

void MatrixSetDownPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
);

void MatrixSetLeftPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
);

void MatrixSetRightPadding(
    const matrix* m,
    const unsigned int border_padding,
    const MATRIX_NNNER_DATA_TYPE padding_value
);

void MatrixInit(
    matrix* m,
    const unsigned int level_r,
    const unsigned int level_c,
    const unsigned int level_number_of_elements,
    const unsigned int border_padding
);