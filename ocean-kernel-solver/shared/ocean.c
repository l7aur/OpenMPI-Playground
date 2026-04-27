#include "util.h"
#include "ocean.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define     LEVEL_PADDING_SIZE      1
#define     OCEAN_LEVEL_SENTINEL    0

void _OceanLevelInit(
    matrix* m
);

void _OceanSubsampleLevel(
    matrix* to_be_sampled,
    matrix* output
);

ocean* OceanAllocate(
    const unsigned int number_of_levels,
    const unsigned int downsampling_rate,
    const unsigned int finest_grid_size
) {
    assert(number_of_levels > 0);
    assert(finest_grid_size & (finest_grid_size - 1) == 0);
    assert(downsampling_rate & (downsampling_rate - 1) == 0);

    ocean * o = (ocean*)malloc(sizeof(ocean) * 1);
    if (o == NULL) {
        fprintf(stderr, "Failed to allocate ocean!\n");
        return NULL;
    }

    o->number_of_levels = number_of_levels;

    __try {
        o->levels = (matrix**)malloc(sizeof(matrix*) * number_of_levels);
        if (o->levels == NULL) {
            fprintf(stderr, "Failed to allocate levels buffer!\n");
            __throw(EXIT_FAILURE);
        }

        unsigned int level_size = finest_grid_size;
        for (unsigned int l = 0; l < o->number_of_levels; l++) {
            o->levels[l] = MatrixAllocate(level_size + LEVEL_PADDING_SIZE * 2, level_size + LEVEL_PADDING_SIZE * 2);
            if (o->levels[l] == NULL) {
                fprintf(stderr, "Failed to allocate level %d\n", l);
                __throw(EXIT_FAILURE);
            }
            level_size >>= downsampling_rate;
        }

    }
    __finally {
        if (__error_code != EXIT_SUCCESS && o != NULL) {
            if (o->levels != NULL) {
                for (unsigned int l = 0; l < o->number_of_levels; l++) {
                    if (o->levels[l] != NULL)
                        { MatrixDeallocate(o->levels[l]); o->levels[l] = NULL; }
                }

                free(o->levels), o->levels = NULL;
            }

            free(o), o = NULL;
        }
    }

    return o;
}

void OceanDeallocate(
    ocean* o
) {
    assert(o != NULL);

    if (o->levels != NULL) {
        for (unsigned int l = 0; l < o->number_of_levels; l++) {
            if (o->levels[l] != NULL)
                { MatrixDeallocate(o->levels[l]); o->levels[l] = NULL; }
        }

        free(o->levels), o->levels = NULL;
    }

    free(o), o = NULL;
}

matrix* OceanLevelAt(
    const ocean* o,
    const unsigned int level_index
) {
    assert(level_index < o->number_of_levels);

    if (o->levels == NULL)
        return NULL;

    return &(o->levels[level_index]);
}

int OceanInit(
    ocean* o
) {
    assert(o != NULL); assert(o->levels != NULL);
    assert(o->number_of_levels > 0);

    _OceanLevelInit(o->levels[0]);

    for (unsigned int l = 1; l < o->number_of_levels; l++) {
        if (OceanSampleLevel(o->levels[l - 1], o->levels[l]) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to subsample at level %d!\n", l - 1);
            return EXIT_FAILURE;
        }
    }
}

void _OceanLevelInit(
    matrix* m
) {
    assert(m != NULL); assert(m->data != NULL);

    MatrixSetPadding(m, LEVEL_PADDING_SIZE, OCEAN_LEVEL_SENTINEL);

    unsigned int r_start = LEVEL_PADDING_SIZE;
    unsigned int r_end = m->rows - LEVEL_PADDING_SIZE;
    unsigned int c_start = LEVEL_PADDING_SIZE;
    unsigned int c_end = m->cols - LEVEL_PADDING_SIZE;
    for (unsigned int r = r_start; r < r_end; r++)
        for (unsigned int c = c_start; c < c_end; c++) {
            double factor = M_PI / (double)(c_end - LEVEL_PADDING_SIZE);
            *MatrixAddressAt(m, r, c) = sinl(r * factor) * sinl(c * factor);
        }

    return EXIT_SUCCESS;
}

/**
 * @brief Subsamples a level to generate another. Both input parameters must be
 * initialized with the required dimensions.
 * @note The ratio between dimensions + 1 represents the subsampling resolution.
 * This method uses an averages subsampling technique: the further away the coordinates
 * from the center of the sampling area, the smaller its weight to the final result.
 * The weights are powers of 2, the center of the sampling area receives a weight of 0.25,
 * further points receive 0.25 * 2^(-d(center, point)).
 * @param to_be_sampled finer ocean level
 * @param output coarser ocean level
 */
void _OceanSubsampleLevel(
    matrix* to_be_sampled,
    matrix* output
) {
    assert(to_be_sampled != NULL); assert(to_be_sampled->data != NULL);
    assert(output != NULL); assert(output->data != NULL);
    assert((to_be_sampled->rows - LEVEL_PADDING_SIZE) & (to_be_sampled->rows - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->cols - LEVEL_PADDING_SIZE) & (to_be_sampled->cols - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((output->rows - LEVEL_PADDING_SIZE) & (output->rows - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((output->cols - LEVEL_PADDING_SIZE) & (output->cols - 1 - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->rows - LEVEL_PADDING_SIZE) % (output->rows - LEVEL_PADDING_SIZE) == 0);
    assert((to_be_sampled->cols - LEVEL_PADDING_SIZE) % (output->cols - LEVEL_PADDING_SIZE) == 0);

    MatrixSetPadding(output, LEVEL_PADDING_SIZE, OCEAN_LEVEL_SENTINEL);

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
                    sample_value += (1 << (2 + abs(i + j))) * (*MatrixAddressAt(to_be_sampled, i, j));

            *MatrixAddressAt(output, r, c) = sample_value;
        }

    return EXIT_SUCCESS;
}