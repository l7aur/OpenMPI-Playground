#pragma once

#include <filesystem>
#include <mpi/mpi.h>

#include "Grid.hpp"

class Worker {
public:
    Worker(
        const MPI_Comm cartesian_comm_,
        const std::pair<int, int>& position,
        const std::filesystem::path& p,
        const float max_diff,
        const std::pair<int, int> grid_size
    );
    void solve();

private:
    float max_diff;
    std::pair<int, int> position;
    Grid grid;
    MPI_Comm cartesian_comm;

    void read(
        const std::filesystem::path& p,
        const std::pair<int, int> grid_size
    );
    void update_interprocess_borders();
    void exchange_row_up(
        const int dest
    );
    void exchange_row_down(
        const int dest
    );
    void exchange_col_left(
        const int dest
    );
    void exchange_col_right(
        const int dest
    );
    void exchange(
        float * send_buffer,
        float * recv_buffer,
        const int n,
        const int dest
    );
};