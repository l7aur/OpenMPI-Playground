#pragma once

int Slave(
    const int id,
    const int coords[WORLD_DIMENSIONS],
    MPI_Comm* cartesianComm
);