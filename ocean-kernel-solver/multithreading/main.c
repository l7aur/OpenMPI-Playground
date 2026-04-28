#include "global.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    __try {
        unsigned int number_of_levels = 0;
        unsigned int downsampling_rate = 0;
        unsigned int finest_grid_size = 0;
        unsigned int number_of_workers = 0;

        if (Parser(
            argc,
            argv,
            &number_of_levels,
            &downsampling_rate,
            &finest_grid_size,
            &number_of_workers
        ) != EXIT_SUCCESS) {
            fprintf(stderr, "Failed to parse the command line arguments!\n");
            __throw(EXIT_FAILURE);
        };

        if ((global_ocean = OceanAllocate(
            number_of_levels,
            downsampling_rate,
            finest_grid_size,
            number_of_workers
        )) == NULL) {
            fprintf(stderr, "Failed to allocate the global ocean!\n");
            return EXIT_FAILURE;
        }

        OceanInit(global_ocean);
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (global_ocean != NULL)
                { OceanDeallocate(global_ocean); global_ocean = NULL; }
        }
    }

    return EXIT_SUCCESS;
}