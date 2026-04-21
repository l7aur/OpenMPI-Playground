#pragma once

#include <stdio.h>

#define MAT_INNER_TYPE              int
#define MAT_STRUCT_NUMBER_OF_FIELDS 3

typedef struct _mat 
{
    unsigned int rows;
    unsigned int cols; 
    MAT_INNER_TYPE* data;
} mat;

mat* MatrixAllocate(
    const unsigned int rows,
    const unsigned int cols
);

void MatrixDeallocate(
    mat** matrix
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

int MatrixPartition(
    const unsigned int gridWidth,
    const unsigned int gridHeight,
    mat** input,
    mat*** output
);