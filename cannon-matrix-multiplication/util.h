#pragma once

#include <stdlib.h>
#include <math.h>

#include <mpi/mpi.h>

#include "mat.h"

#define CUSTOM_DEBUG

#define WORLD_DIMENSIONS        2
#define MPI_ALLOW_REORDER       1
#define MAX_NUMBER_OF_MATRICES  2

#define MATRIX_A_TAG            10
#define MATRIX_B_TAG            20

#define GRID_DIMENSION(X)       sqrt(X)

#define __try           int __error_code = EXIT_SUCCESS;       
#define __throw(X)      { __error_code = (X); goto final; }
#define __finally       final:

enum Direction { Left, Right, Up, Down };

int Parse(
    const int argc,
    const char* argv[],
    mat** matrix_a /* OUT */,
    mat** matrix_b /* OUT */
);