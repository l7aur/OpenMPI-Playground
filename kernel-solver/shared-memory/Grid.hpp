#pragma once

#include <vector>
#include <memory>
#include <filesystem>


class Grid {
public:
    Grid();

    void read(
        const std::filesystem::path& p
    );
    void setup_for_new_iteration();
    float solve(
        const int r_start,
        const int c_start,
        const int r_end,
        const int c_end
    );
    void print_write_buffer() const;
    void print_read_buffer() const;
    int get_grid_size() const;

private:
    std::unique_ptr<std::vector<std::vector<float>>> data_read_buffer;
    std::unique_ptr<std::vector<std::vector<float>>> data_write_buffer;
    int rows;
    int cols;
};