#pragma once

#include <filesystem>
#include <mpi/mpi.h>

#include "Grid.hpp"

class Worker {
public:
    Worker(
        const MPI_Comm cartesian_comm_,
        const int rank,
        const std::pair<int, int>& position,
        const std::filesystem::path& p,
        const float max_diff,
        const std::pair<int, int> grid_size
    );
    void solve(
        const int world_size,
        const float max_error
    );

private:
    float max_diff;
    std::pair<int, int> position;
    Grid grid;
    MPI_Comm cartesian_comm;
    int rank;

    void read(
        const std::filesystem::path& p,
        const std::pair<int, int> grid_size
    );
    void update_interprocess_borders();
    void exchange_vertically(
        const int above,
        const int below
    );
    void exchange_horizontally(
        const int left,
        const int right
    );
    void exchange(
        float * send_buffer,   
        const int send_dest,
        float * recv_buffer,
        const int recv_source,
        const int n
    );
};