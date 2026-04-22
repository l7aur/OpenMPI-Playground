#pragma once

int Slave(
    const int id,
    const unsigned int gridLength,
    const int coords[WORLD_DIMENSIONS],
    MPI_Comm* cartesianComm
);