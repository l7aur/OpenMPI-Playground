#pragma once

#include <stdlib.h>
#include <math.h>

#include <mpi/mpi.h>

#include "mat.h"

// #define CUSTOM_DEBUG
#define CUSTOM_DEBUG_ALG

#define WORLD_DIMENSIONS        2
#define MPI_ALLOW_REORDER       1
#define MAX_NUMBER_OF_MATRICES  2

#define MATRIX_A_TAG            10
#define MATRIX_B_TAG            20

#define __try           int __error_code = EXIT_SUCCESS;       
#define __throw(X)      { __error_code = (X); goto final; }
#define __finally       final:

typedef enum Direction_ { Left, Right, Up, Down } Direction;

int Parse(
    const int argc,
    const char* argv[],
    const unsigned int gridLength,
    mat** matrix_a /* OUT */,
    mat** matrix_b /* OUT */
);

int CanonAlgorithm(
    const int id,
    mat* a, 
    mat* b,
    const int numberOfIterations,
    MPI_Comm* cartesianComm,
    mat** c /* OUT */
);

int CollectResult(

);