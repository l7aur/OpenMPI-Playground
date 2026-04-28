#include "util.h"
#include "ocean.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <math.h>

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

    return (o->levels[level_index]);
}

void OceanInit(
    ocean* o
) {
    assert(o != NULL); assert(o->levels != NULL);
    assert(o->number_of_levels > 0);

    LevelInit(o->levels[0], OCEAN_BORDER_SENTINEL);

    for (unsigned int l = 1; l < o->number_of_levels; l++)
        LevelDownsampleLevel(o->levels[l - 1], o->levels[l], OCEAN_BORDER_SENTINEL);
}