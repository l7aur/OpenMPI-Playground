#include "util.h"
#include "ocean.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <math.h>

void _OceanExchangeLevelBorders(
    ocean * o
);

ocean* OceanAllocate(
    const unsigned int number_of_levels,
    const unsigned int downsampling_rate,
    const unsigned int finest_grid_size,
    const unsigned int grid_size
) {
    assert(number_of_levels > 0);
    assert(downsampling_rate > 1);
    assert((finest_grid_size & (finest_grid_size - 1)) == 0);
    assert((downsampling_rate & (downsampling_rate - 1)) == 0);
    assert((finest_grid_size >> ((number_of_levels - 1) * (unsigned int)log2(downsampling_rate))) > 0);
    assert(grid_size > 0);

    ocean * o = (ocean*)malloc(sizeof(ocean) * 1);
    if (o == NULL) {
        fprintf(stderr, "Failed to allocate ocean!\n");
        return NULL;
    }

    o->number_of_levels = number_of_levels;

    __try {
        o->levels = (level**)malloc(sizeof(level*) * number_of_levels);
        if (o->levels == NULL) {
            fprintf(stderr, "Failed to allocate ocean levels buffer!\n");
            __throw(EXIT_FAILURE);
        }

        unsigned int level_size = finest_grid_size;
        for (unsigned int l = 0; l < o->number_of_levels; l++) {
            printf("%d<-\n", level_size);
            o->levels[l] = LevelAllocate(
                level_size,
                level_size,
                grid_size
            );
            if (o->levels[l] == NULL) {
                fprintf(stderr, "Failed to allocate level %d\n", l);
                __throw(EXIT_FAILURE);
            }
            level_size /= downsampling_rate;
        }

    }
    __finally {
        if (__error_code != EXIT_SUCCESS && o != NULL) {
            if (o->levels != NULL) {
                for (unsigned int l = 0; l < o->number_of_levels; l++) {
                    if (o->levels[l] != NULL)
                        { LevelDeallocate(o->levels[l]); o->levels[l] = NULL; }
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
                { LevelDeallocate(o->levels[l]); o->levels[l] = NULL; }
        }

        free(o->levels), o->levels = NULL;
    }

    free(o), o = NULL;
}

level* OceanLevelAt(
    const ocean* o,
    const unsigned int level_index
) {
    assert(level_index < o->number_of_levels);

    if (o->levels == NULL)
        return NULL;

    return o->levels[level_index];
}

void OceanInit(
    ocean* o
) {
    assert(o != NULL); assert(o->levels != NULL);
    assert(o->number_of_levels > 0);

    LevelInit(o->levels[0], OCEAN_BORDER_SENTINEL);

    for (unsigned int l = 1; l < o->number_of_levels; l++)
        LevelDownsampleLevel(OceanLevelAt(o, l - 1), OceanLevelAt(o, l), OCEAN_BORDER_SENTINEL);

    _OceanExchangeLevelBorders(o);
}

void _OceanExchangeLevelBorders(
    ocean * o
) {
    assert(o != NULL);

    for (unsigned int l = 0; l < o->number_of_levels; l++) {
        level* current_level = OceanLevelAt(o, l);
        
        if (current_level->grid_size <= 1)
            break /* smaller grid sizes are coming */;

        for (unsigned int r = 0; r < current_level->grid_size - 1; r++)
            for (unsigned int c = 0; c < current_level->grid_size; c++)
                MatrixCopyLastRowToPadding(
                    LevelMatrixAt(current_level, r, c),
                    LevelMatrixAt(current_level, r + 1, c)
                );

        for (unsigned int r = 1; r < current_level->grid_size; r++)
            for (unsigned int c = 0; c < current_level->grid_size; c++)
                MatrixCopyFirstRowToPadding(
                    LevelMatrixAt(current_level, r, c),
                    LevelMatrixAt(current_level, r - 1, c)
                );

        for (unsigned int r = 0; r < current_level->grid_size; r++)
            for (unsigned int c = 0; c < current_level->grid_size - 1; c++)
                MatrixCopyLastColumnToPadding(
                    LevelMatrixAt(current_level, r, c),
                    LevelMatrixAt(current_level, r, c + 1)
                );

        for (unsigned int r = 0; r < current_level->grid_size; r++)
            for (unsigned int c = 1; c < current_level->grid_size; c++)
                MatrixCopyFirstColumnToPadding(
                    LevelMatrixAt(current_level, r, c),
                    LevelMatrixAt(current_level, r, c - 1)
                );
    }
}