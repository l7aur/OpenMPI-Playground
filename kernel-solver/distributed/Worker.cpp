#include "Worker.hpp"

#include <fstream>
#include <cmath>
#include <cassert>
#include <iostream>

Worker::Worker(
    const MPI_Comm cartesian_comm_,
    const int rank_,
    const std::pair<int, int>& position_,
    const std::filesystem::path &p,
    const float max_diff_,
    const std::pair<int, int> grid_size
)   : max_diff{ max_diff_ }
    , position{ position_ }
    , cartesian_comm{ cartesian_comm_ }
    , rank{ rank_ }
{
    read(p, grid_size);
}

void Worker::read(
    const std::filesystem::path &p,
    const std::pair<int, int> grid_size
)
{
    std::ifstream fin(p);
    if (!fin.is_open())
        throw std::runtime_error("Failed to open file: " + p.string());
    
    int r, c;
    fin >> r >> c;
    assert(r == c);

    fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    const int& grid_height = grid_size.first;
    const int& grid_width = grid_size.second;

    auto my_base_r = r / grid_height;
    auto my_base_c = c / grid_width;
    auto my_r = my_base_r;
    auto my_c = my_base_c;

    if (position.first == grid_height - 1)
        my_r += r % grid_height; 
    if (position.second == grid_width - 1)
        my_c += c % grid_width;

    grid = Grid(my_r, my_c);

    for (int i = 0; i < position.first * my_base_r; i++)
        fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    for (int i = 0; i < my_r; i++) {
        for (int j = 0; j < position.second * my_base_c; j++) {
            float x;
            fin >> x;
        }

        for (int j = 0; j < my_c; j++) {
            fin >> grid.at(i, j);
        }
        fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    fin.close();
}

void Worker::solve()
{
    update_interprocess_borders();
    grid.print(position);
}

void Worker::update_interprocess_borders()
{
    int dirs[2] = { 0 /* vertical */, 1 /* horizontal */ };
    int disp = 1;
    int sources[2];
    int dests[2];
    for (int i = 0; i < 2; i++) {
        auto status = MPI_Cart_shift(
            cartesian_comm,
            dirs[i],
            disp,
            &sources[i],
            &dests[i]
        );
        if (status != MPI_SUCCESS)
            throw std::runtime_error("Failed to find neighbor up");
    }

    exchange_vertically(sources[0], dests[0]);
    exchange_horizontally(sources[1], dests[1]);
}

void Worker::exchange_vertically(
    const int above,
    const int below
)
{
    auto n = grid.get_cols();
    auto first_padding_row = std::make_unique<float[]>(n);
    auto last_padding_row = std::make_unique<float[]>(n);
    
    auto first_data_row = grid.get_first_data_row();
    auto last_data_row = grid.get_last_data_row();

    exchange(first_data_row.get(), above, last_padding_row.get(), below, n);
    exchange(last_data_row.get(), below, first_padding_row.get(), above, n);

    grid.set_first_padding_row(first_padding_row.get());
    grid.set_last_padding_row(last_padding_row.get());
}

void Worker::exchange_horizontally(
    const int left,
    const int right
)
{
    auto n = grid.get_rows();
    auto first_padding_col = std::make_unique<float[]>(n);
    auto last_padding_col = std::make_unique<float[]>(n);

    auto first_data_col = grid.get_first_data_col();
    auto last_data_col = grid.get_last_data_col();

    exchange(first_data_col.get(), left, last_padding_col.get(), right, n);
    exchange(last_data_col.get(), right, first_padding_col.get(), left, n);

    grid.set_first_padding_col(first_padding_col.get());
    grid.set_last_padding_col(last_padding_col.get());
}

void Worker::exchange(
    float * send_buffer,
    const int send_dest,
    float * recv_buffer,
    const int recv_source,
    const int n
)
{
    auto status = MPI_Sendrecv(
        send_buffer,
        n,
        MPI_FLOAT,
        send_dest,
        0,
        recv_buffer,
        n,
        MPI_FLOAT,
        recv_source,
        0,
        cartesian_comm,
        MPI_STATUS_IGNORE
    );
    if (status != MPI_SUCCESS)
        throw std::runtime_error("Failed to send recv");
}