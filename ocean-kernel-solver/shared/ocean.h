#pragma once

#include "level.h"

#define     OCEAN_BORDER_SENTINEL   -1

typedef struct _ocean {
    level** levels;
    unsigned int number_of_levels;
} ocean;

ocean* OceanAllocate(
    const unsigned int number_of_levels,
    const unsigned int downsampling_rate,
    const unsigned int finest_grid_size,
    const unsigned int grid_size
);

void OceanDeallocate(
    ocean* o
);

matrix* OceanLevelAt(
    const ocean* o,
    const unsigned int level_index
);

int OceanInit(
    ocean* o
);