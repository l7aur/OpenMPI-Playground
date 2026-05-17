#pragma once

#include <vector>
#include <memory>

class Grid {
public:
    constexpr static inline const int PADDING{ 1 };   

    Grid() = default;
    Grid(
        const int rows,
        const int cols
    );
    float& at(
        const int r,
        const int c
    );
    void print(
        const std::pair<int, int>& grid_pos
    );
    std::unique_ptr<float[]> get_first_data_row() const;
    void set_first_padding_row(
        const float elems[]
    );
    std::unique_ptr<float[]> get_last_data_row() const;
    void set_last_padding_row(
        const float elems[]
    );
    std::unique_ptr<float[]> get_first_data_col() const;
    void set_first_padding_col(
        const float elems[]
    );
    std::unique_ptr<float[]> get_last_data_col() const;
    void set_last_padding_col(
        const float elems[]
    );
    int get_cols() const { return cols - 2 * PADDING; }
    int get_rows() const { return rows - 2 * PADDING; }

private:
    std::vector<float> data;
    int rows;
    int cols;
};