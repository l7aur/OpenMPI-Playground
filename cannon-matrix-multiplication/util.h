#pragma once

#include <stdlib.h>
#include <mpi/mpi.h>

#include "mat.h"

#define CUSTOM_DEBUG

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