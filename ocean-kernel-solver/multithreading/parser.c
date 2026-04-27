#include "parser.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int Parser(
    const int argc,
    char * argv[],
    unsigned int* number_of_levels,
    unsigned int* downsampling_rate,
    unsigned int* finest_grid_size,
    unsigned int* number_of_workers
) {
    assert(argc > 0);
    assert(argv != NULL);

    if (argc != 5) {
        fprintf(stderr, "Usage ./main <number_of_workers> <number_of_levels>, <downsampling_rate> <finest_grid_size>\n");
        return EXIT_FAILURE;
    }

    *number_of_levels = atoi(argv[1]);
    *downsampling_rate = atoi(argv[2]);
    *finest_grid_size = atoi(argv[3]);
    *number_of_workers = atoi(argv[4]);

    if (*number_of_levels < 0) {
        fprintf(stderr, "<number_of_levels> must be greater than 0!\n");
        return EXIT_FAILURE;
    }

    if (*downsampling_rate < 1) {
        fprintf(stderr, "<downsampling_rate> must be greater than 1!\n");
        return EXIT_FAILURE;
    }

    if ((*downsampling_rate & (*downsampling_rate - 1)) != 0) {
        fprintf(stderr, "<downsampling_rate> must be a power of 2!\n");
        return EXIT_FAILURE;
    }

    if (*finest_grid_size < 1) {
        fprintf(stderr, "<finest_grid_size> must be greater than 1!\n");
        return EXIT_FAILURE;
    }

    if ((*finest_grid_size & (*finest_grid_size - 1)) != 0) {
        fprintf(stderr, "<finest_grid_size> must be a power of 2!\n");
        return EXIT_FAILURE;
    }

    unsigned int coarser_grid_size = (*finest_grid_size) >> ((*number_of_levels - 1) * (unsigned int)log2(*downsampling_rate));
    if (coarser_grid_size < 1) {
        fprintf(stderr,
            "The provided arguments cannot generate an ocean! Unable to downsample a"
            "%dx%d matrix by %d for %d times and still obtain a matrix\n",
            *finest_grid_size, *finest_grid_size, *downsampling_rate, *number_of_levels
        );
        return EXIT_FAILURE;
    }

    if (*number_of_workers < 1) {
        fprintf(stderr, "<number_of_workers> must be greater than 0!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}