#pragma once

#include "matrix.h"

typedef struct _level {
    matrix** data;
    unsigned int rows;
    unsigned int cols;
} level;

level* LevelAllocate(
    const unsigned int rows,
    const unsigned int cols,
    const unsigned int matrix_rows,
    const unsigned int matrix_cols
);

void LevelDeallocate(
    level* l
);

matrix* LevelMatrixAt(
    const level * l,
    const unsigned int r,
    const unsigned int c
);

void LevelInit(
    level* l,
    const MATRIX_NNNER_DATA_TYPE ocean_border_value
);

/**
 * @brief Downsamples a level to generate another. Both input parameters must be
 * initialized with the required dimensions.
 * @note The ratio between dimensions + 1 represents the subsampling resolution.
 * This method uses an averages downsampling technique: the further away the coordinates
 * from the center of the sampling area, the smaller its weight to the final result.
 * The weights are powers of 2, the center of the sampling area receives a weight of 0.25,
 * further points receive 0.25 * 2^(-d(center, point)).
 * @param to_be_sampled finer ocean level
 * @param output coarser ocean level
 */
void LevelDownsampleLevel(
    level* to_be_sampled,
    level* output,
    const MATRIX_NNNER_DATA_TYPE ocean_border_value
);