#pragma once

#include <stdio.h>

#define MAT_INNER_TYPE      int

typedef struct _mat 
{
    int rows;
    int cols; 
    MAT_INNER_TYPE* data;
} mat;

mat* MatrixAllocate(
    const int rows,
    const int cols
);

void MatrixDeallocate(
    mat* matrix
);

mat* MatrixRead(
    const char* file_path
);

MAT_INNER_TYPE* MatrixAt(
    const mat* matrix,
    const unsigned int row,
    const unsigned int col
);

void MatrixPrint(
    const mat* matrix,
    FILE* out
);