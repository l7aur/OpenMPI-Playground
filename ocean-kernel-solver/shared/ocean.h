#pragma once

#include "matrix.h"

typedef struct _ocean {
    matrix** levels;
    unsigned int number_of_levels;
} ocean;

ocean* OceanAllocate(
    const unsigned int number_of_levels,
    const unsigned int downsampling_rate,
    const unsigned int finest_grid_size
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