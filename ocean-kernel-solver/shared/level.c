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
    assert((to_be_sampled->rows - LEVEL_PADDING_SIZE) & (to_be_sampled->rows - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->cols - LEVEL_PADDING_SIZE) & (to_be_sampled->cols - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((output->rows - LEVEL_PADDING_SIZE) & (output->rows - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((output->cols - LEVEL_PADDING_SIZE) & (output->cols - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->rows - LEVEL_PADDING_SIZE) % (output->rows - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->cols - LEVEL_PADDING_SIZE) % (output->cols - LEVEL_PADDING_SIZE) == 0);

    unsigned int r_start = LEVEL_PADDING_SIZE;
    unsigned int r_end = output->rows - LEVEL_PADDING_SIZE;
    unsigned int c_start = LEVEL_PADDING_SIZE;
    unsigned int c_end = output->cols - LEVEL_PADDING_SIZE;
    unsigned int r_resolution = 1 + (to_be_sampled->rows - LEVEL_PADDING_SIZE * 2) / (output->rows - LEVEL_PADDING_SIZE * 2);
    unsigned int c_resolution = 1 + (to_be_sampled->cols - LEVEL_PADDING_SIZE * 2) / (output->cols - LEVEL_PADDING_SIZE * 2);
    for (unsigned int r = r_start; r < r_end; r++)
        for (unsigned int c = c_start; c < c_end; c++) {
            unsigned int r_sample_center = (r - LEVEL_PADDING_SIZE) * r_resolution + LEVEL_PADDING_SIZE;
            unsigned int c_sample_center = (c - LEVEL_PADDING_SIZE) * c_resolution + LEVEL_PADDING_SIZE;

            MATRIX_NNNER_DATA_TYPE sample_value = 0.0;
            for (unsigned int i = -r_resolution / 2; i <= r_resolution / 2; i++)
                for (unsigned int j = -c_resolution / 2; j <= c_resolution / 2; j++)
                    sample_value += 1.0 / (1 << (2 + abs(i) + abs(j))) * (*MatrixAddressAt(to_be_sampled, i, j));

            *MatrixAddressAt(output, r, c) = sample_value;
        }
}