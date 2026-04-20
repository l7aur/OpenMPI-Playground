#include "mat.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

mat* MatrixAllocate(
    const int rows,
    const int cols
) {
    assert(rows > 0 && cols > 0);

    mat * matrix = (mat*)malloc(1 * sizeof(mat));
    if (matrix == NULL) {
        fprintf(stderr, "Failed to allocate matrix!\n");
        return NULL;
    }

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = (MAT_INNER_TYPE*)malloc(sizeof(MAT_INNER_TYPE) * rows * cols);
    if (matrix->data == NULL) {
        fprintf(stderr, "Failed to allocate matrix data!\n");
        free(matrix), matrix = NULL;
        return NULL;
    }

    return matrix;
}

void MatrixDeallocate(
    mat* matrix
) {
    assert(matrix != NULL);
    
    if (matrix->data != NULL)
        free(matrix->data), matrix->data = NULL;
    
    free(matrix), matrix = NULL;
}

mat* MatrixRead(
    const char* file_path
) {
    FILE* fd = fopen(file_path, "r");
    if (fd == NULL) {
        fprintf(stderr, "Failed to open input file '%s'.\n", file_path);
        return NULL;
    }

    mat* matrix = NULL;
    __try {
        int cols = 0, rows = 0;
        int bytes_read = fscanf(fd, "%d %d\n", &rows, &cols);
        if (bytes_read != 2) {
            fprintf(stderr, "Failed to read number of rows and columns for matrix in '%s'.\n", file_path);
            __throw(EXIT_FAILURE);
        }

        matrix = MatrixAllocate(rows, cols);
        if (matrix == NULL) {
            fprintf(stderr, "Failed to allocate matrix!\n");
            __throw(EXIT_FAILURE);
        }

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                bytes_read = fscanf(fd, "%d", MatrixAt(matrix, i, j));
                if (bytes_read != 1) {
                    fprintf(stderr, "Failed to read matrix element at (%d; %d)\n", i, j);
                    __throw(EXIT_FAILURE);
                }
            }
        }
    }
    __finally {
        if (__error_code != EXIT_SUCCESS) {
            if (fd != NULL)
                fclose(fd);

            if (matrix != NULL)
                MatrixDeallocate(matrix);
            return NULL;
        }
    }

    return matrix;
}

MAT_INNER_TYPE* MatrixAt(
    const mat* matrix,
    const unsigned int row,
    const unsigned int col
) {
    assert(matrix != NULL);
    assert(row >= 0 && col >= 0);
    assert(row < matrix->rows && col < matrix->cols);

    return &(matrix->data[row * matrix->cols + col]);
}

void MatrixPrint(
    const mat* matrix,
    FILE* out
) {
    assert(matrix != NULL);
    assert(out != NULL);

    for (unsigned int i = 0; i < matrix->rows; i++) {
        for (unsigned int j = 0; j < matrix->cols; j++)
            fprintf(out, "%d ", *MatrixAt(matrix, i, j));
        fprintf(out, "\n");
    }
}