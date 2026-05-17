#pragma once

#include <mpi/mpi.h>

class MPIContext {
public:
    MPIContext();
    ~MPIContext();

    std::pair<int, int> get_position() const { return { position[0], position[1] }; }
    std::pair<int, int> get_grid_size() const { return grid_size; }
    MPI_Comm get_cartesian_comm() const { return cartesian_comm; }
    
private:
    std::pair<int, int> grid_size;
    int rank;
    int position[2]{ 0, 0 };
    int dims[2]{ 0, 0 };
    int periods[2]{ 0, 0 };
    MPI_Comm cartesian_comm{ MPI_COMM_NULL };
    
    void init();
};