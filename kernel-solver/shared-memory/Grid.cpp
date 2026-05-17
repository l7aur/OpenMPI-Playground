#include "Grid.hpp"

#include <cmath>
#include <fstream>
#include <cassert>
#include <iostream>

namespace
{
    constexpr const auto PADDING{ 1 };  
    constexpr const auto UNIFORMIZATION_FACTOR{ 0.2f };
}

Grid::Grid()
    : data_read_buffer(std::make_unique<std::vector<std::vector<float>>>())
    , data_write_buffer(std::make_unique<std::vector<std::vector<float>>>())
{
}

void Grid::read(
    const std::filesystem::path &p)
{
    std::ifstream fin(p);
    if (!fin.is_open())
        throw std::runtime_error("Failed to locate file: " + p.string());

    fin >> rows >> cols;

    if (rows <= 0 || cols <= 0)
        throw std::runtime_error("Expected non-null and non-negative values for rows and cols");
   
    rows += 2 * PADDING;
    cols += 2 * PADDING;

    data_read_buffer->resize(rows);
    for (auto& row : *data_read_buffer) {
        row = std::vector<float>(cols);
    }
    
    data_write_buffer->resize(rows);
    for (auto& row : *data_write_buffer) {
        row = std::vector<float>(cols);
    }

    for (int i = 0; i < rows; i++) {
        data_read_buffer->at(i).at(0) = data_read_buffer->at(i).at(cols - 1) = 0.0f;
        data_write_buffer->at(i).at(0) = data_write_buffer->at(i).at(cols - 1) = 0.0f;
    }

    for (int j = 0; j < cols; j++) {
        data_read_buffer->at(0).at(j) = data_read_buffer->at(rows - 1).at(j) = 0.0f;
        data_write_buffer->at(0).at(j) = data_write_buffer->at(rows - 1).at(j) = 0.0f;
    }

    for (int i = PADDING; i < rows - PADDING; i++)
        for (int j = PADDING; j < cols - PADDING; j++)
            fin >> data_write_buffer->at(i).at(j);

    fin.close();

    setup_for_new_iteration();
}

void Grid::setup_for_new_iteration()
{
    data_read_buffer.swap(data_write_buffer);
}

float Grid::solve(
    const int r_start,
    const int c_start,
    const int r_end,
    const int c_end
)
{
    float diff = 0.0f;
    for (auto i = r_start + PADDING; i < r_end + PADDING; i++) {
        for (auto j = c_start + PADDING; j < c_end + PADDING; j++) {
            data_write_buffer->at(i).at(j) = UNIFORMIZATION_FACTOR * (
                data_read_buffer->at(i).at(j) +
                data_read_buffer->at(i - 1).at(j) + 
                data_read_buffer->at(i + 1).at(j) +
                data_read_buffer->at(i).at(j + 1) + 
                data_read_buffer->at(i).at(j - 1)
            );
            diff += std::abs(data_read_buffer->at(i).at(j) - data_write_buffer->at(i).at(j));
        }
    }
    return diff;
}

void Grid::print_write_buffer() const
{
    for (const auto& r : *data_write_buffer) {
        for (const auto& el : r)
            std::cout << el << " ";
        std::cout << std::endl;
    }
}

void Grid::print_read_buffer() const
{
    for (const auto& r : *data_read_buffer) {
        for (const auto& el : r)
            std::cout << el << " ";
        std::cout << std::endl;
    }
}

int Grid::get_grid_size() const
{
    return cols - 2 * PADDING; 
}
