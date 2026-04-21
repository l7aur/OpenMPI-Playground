#pragma once

int Master(
    const int id,
    const unsigned int gridWidth,
    const unsigned int gridHeight,
    const int coords[WORLD_DIMENSIONS],
    mat** matrix_a,
    mat** matrix_b,
    MPI_Comm* cartesianComm
);