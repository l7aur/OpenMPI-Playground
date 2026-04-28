#include "util.h"
#include "level.h"

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define     LEVEL_PADDING_SIZE      1

level* LevelAllocate(
    const unsigned int rows,
    const unsigned int cols,
    const unsigned int matrix_row,
    const unsigned int matrix_col
) {
    assert(rows > 0); assert(cols > 0);

    level* l = (level*)malloc(sizeof(level) * 1);
    if (l == NULL) {
        fprintf(stderr, "Failed to allocate level buffer!\n");
        return EXIT_FAILURE;
    }

    l->cols = cols + 2 * LEVEL_PADDING_SIZE;
    l->rows = rows + 2 * LEVEL_PADDING_SIZE;

    __try {
        l->data = (matrix**)malloc(sizeof(matrix*) * l->rows * l->cols);
        if (l->data == NULL) {
            fprintf(stderr, "Failed to allocate level matrix buffer!\n");
            __throw(EXIT_FAILURE);
        }

        for (unsigned int m = 0; m < l->cols * l->rows; m++) {
            l->data[m] = MatrixAllocate(
                matrix_row + 2 * LEVEL_PADDING_SIZE,
                matrix_col + 2 * LEVEL_PADDING_SIZE
            );
            if (l->data[m] == NULL) {
                fprintf(stderr, "Failed to allocate matrix in level matrix buffer!\n");
                __throw(EXIT_FAILURE);
            }
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (l->data != NULL) {
                for (unsigned int m = 0; m < l->cols * l->rows; m++)
                    MatrixDeallocate(l->data[m]), l->data[m] = NULL;

                free(l->data), l->data = NULL;
            }
            free(l), l = NULL;
        }
    }

    return l;
}

void LevelDeallocate(
    level* l
) {
    assert(l != NULL);

    if (l->data != NULL) {
        for (unsigned int m = 0; m < l->cols * l->rows; m++)
            MatrixDeallocate(l->data[m]), l->data[m] = NULL;

        free(l->data), l->data = NULL;
    }

    free(l), l = NULL;
}

matrix* LevelMatrixAt(
    const level * l,
    const unsigned int r,
    const unsigned int c
) {
    assert(l != NULL);
    assert(r < l->rows); assert(c < l->cols);

    if (l->data == NULL)
        return NULL;

    return l->data[r * l->cols + c];
}

MATRIX_NNNER_DATA_TYPE* LevelValueAt(
    const level* l,
    const unsigned int r,
    const unsigned int c
) {
    assert(l != NULL); assert(l->data != NULL);
    assert(r < l->rows); assert(c < l->cols);

    const matrix* any_matrix = LevelMatrixAt(l, 0, 0);
    const unsigned int matrix_r = any_matrix->rows - 2 * LEVEL_PADDING_SIZE;
    const unsigned int matrix_c = any_matrix->cols - 2 * LEVEL_PADDING_SIZE;

    const unsigned int level_r = r / matrix_r;
    const unsigned int level_c = c / matrix_c;

    return MatrixAddressAt(LevelMatrixAt(l, level_r, level_c), r % matrix_r + LEVEL_PADDING_SIZE, c % matrix_c + LEVEL_PADDING_SIZE);
}

void LevelInit(
    level* l,
    const MATRIX_NNNER_DATA_TYPE ocean_border_value
) {
    assert(l != NULL); assert(l->data != NULL);
    assert(LEVEL_PADDING_SIZE > 0);
    assert(LEVEL_PADDING_SIZE * 2 < l->cols);
    assert(LEVEL_PADDING_SIZE * 2 < l->rows);

    for (unsigned int r = 0; r < l->rows; r++) {
        MatrixSetLeftPadding(LevelMatrixAt(l, r, 0), LEVEL_PADDING_SIZE, ocean_border_value);
        MatrixSetRightPadding(LevelMatrixAt(l, r, l->cols - 1), LEVEL_PADDING_SIZE, ocean_border_value);
    }
    for (unsigned int c = 0; c < l->cols; c++) {
        MatrixSetUpPadding(LevelMatrixAt(l, 0, c), LEVEL_PADDING_SIZE, ocean_border_value);
        MatrixSetDownPadding(LevelMatrixAt(l, l->rows - 1, c), LEVEL_PADDING_SIZE, ocean_border_value);
    }

    unsigned int level_r = 0; // do not count border
    unsigned int level_c = 0; // do not count border
    for (unsigned int r = 0; r < l->rows; r++) {
        for (unsigned int c = 0; c < l->cols; c++) {
            assert(LevelMatrixAt(l, 0, 0)->cols = LevelMatrixAt(l, r, c)->cols);
            assert(LevelMatrixAt(l, 0, 0)->rows = LevelMatrixAt(l, r, c)->rows);

            matrix* current = LevelMatrixAt(l, r, c);
            MatrixInit(current, level_r, level_c, l->rows * l->cols, LEVEL_PADDING_SIZE);
            level_c += current->cols - 2 * LEVEL_PADDING_SIZE;
        }
        level_r += LevelMatrixAt(l, 0, 0)->rows - 2 * LEVEL_PADDING_SIZE;
        level_c = 0;
    }

    return EXIT_SUCCESS;
}

void LevelDownsampleLevel(
    level* to_be_sampled,
    level* output,
    const MATRIX_NNNER_DATA_TYPE ocean_border_value
) {
    assert(to_be_sampled != NULL); assert(to_be_sampled->data != NULL);
    assert(output != NULL); assert(output->data != NULL);
    assert(to_be_sampled->rows & (to_be_sampled->rows - 1) == 0);
    assert(to_be_sampled->cols & (to_be_sampled->cols - 1) == 0);
    assert(output->rows & (output->rows - 1) == 0);
    assert(output->cols & (output->cols - 1) == 0);
    assert(to_be_sampled->rows % output->rows == 0);
    assert(to_be_sampled->cols % output->cols == 0);

    unsigned int r_resolution = 1 + (to_be_sampled->rows) / (output->rows);
    unsigned int c_resolution = 1 + (to_be_sampled->cols) / (output->cols);
    for (unsigned int r = r_resolution; r < output->rows - r_resolution; r++)
        for (unsigned int c = c_resolution; c < output->cols - c_resolution; c++) {

            MATRIX_NNNER_DATA_TYPE sample_value = 0.0;
            for (int i = -r_resolution / 2; i < r_resolution / 2; r++)
                for (int j = -c_resolution / 2; j < c_resolution / 2; c++)
                    sample_value += 1.0 / (1 << (2 + abs(i) + abs(j))) * (*LevelValueAt(to_be_sampled, r + i, c + j));

            *LevelValueAt(output, r - r_resolution, c - c_resolution) = sample_value;
        }
}