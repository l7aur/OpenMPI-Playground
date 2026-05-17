#include "Grid.hpp"

#include <iostream>
#include <sstream>

Grid::Grid(
    const int rows_,
    const int cols_
)   : rows{ rows_ + 2 * PADDING }
    , cols{ cols_ + 2 * PADDING}
{
    data.resize(rows * cols);
}

float &Grid::at(
    const int r,
    const int c
)
{
    return data.at((r + PADDING) * cols + (c + PADDING));
}

void Grid::print(
    const std::pair<int, int>& grid_pos
)
{
    std::stringstream ss;
    ss << grid_pos.first << ' ' << grid_pos.second << std::endl;
    for (int i = 0; i < rows; i++, ss << std::endl)
        for (int j = 0; j < cols; j++)
            ss << data.at(i * cols + j) << ' ';
    ss << std::endl;

    std::cout << ss.str();
}

std::unique_ptr<float[]> Grid::get_first_data_row() const
{
    auto elems = std::make_unique<float[]>(cols - 2 * PADDING);
    for (int i = PADDING; i < cols - PADDING; i++)
        elems[i - PADDING] = data.at(PADDING * cols + i);
    return elems;
}

void Grid::set_first_padding_row(
    const float elems[]
)
{
    for (int i = PADDING; i < cols - PADDING; i++)
        data.at(i) = elems[i - PADDING];
}

std::unique_ptr<float[]> Grid::get_last_data_row() const
{
    auto elems = std::make_unique<float[]>(cols - 2 * PADDING);
    for (int i = PADDING; i < cols - PADDING; i++)
        elems[i - PADDING] = data.at((rows - PADDING - 1) * cols + i);
    return elems;
}

void Grid::set_last_padding_row(
    const float elems[]
)
{
    for (int i = PADDING; i < cols - PADDING; i++)
        data.at((rows - 1) * cols + i) = elems[i - PADDING];
}

std::unique_ptr<float[]> Grid::get_first_data_col() const
{
    auto elems = std::make_unique<float[]>(rows - 2 * PADDING);
    for (int i = PADDING; i < rows - PADDING; i++)
        elems[i - PADDING] = data.at(i * cols + PADDING);
    return elems;
}

void Grid::set_first_padding_col(
    const float elems[]
)
{
    for (int i = PADDING; i < rows - PADDING; i++)
        data.at(i * cols) = elems[i - PADDING];
}

std::unique_ptr<float[]> Grid::get_last_data_col() const
{
    auto elems = std::make_unique<float[]>(rows - 2 * PADDING);
    for (int i = PADDING; i < rows - PADDING; i++)
        elems[i - PADDING] = data.at(i * cols + cols - PADDING - 1);
    return elems;
}

void Grid::set_last_padding_col(
    const float elems[]
)
{
    for (int i = PADDING; i < rows - PADDING; i++)
        data.at(i * cols + cols - 1) = elems[i - PADDING];
}