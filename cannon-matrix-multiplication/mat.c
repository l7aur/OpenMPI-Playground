#include "mat.h"
#include "util.h"

#include <mpi/mpi.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <memory.h>

mat* _MatrixCloneSubMatrix(
    const mat* input,
    const unsigned int rowStart,
    const unsigned int colStart,
    const unsigned int rowEnd,
    const unsigned int colEnd
);

mat* MatrixAllocate(
    const unsigned int rows,
    const unsigned int cols
) {
    assert(rows > 0 && cols > 0);

    mat * matrix = (mat*)malloc(1 * sizeof(mat));
    if (matrix == NULL) {
        fprintf(stderr, "Failed to allocate matrix!\n");
        return NULL;
    }

    matrix->rows = rows;
    matrix->cols = cols;
    matrix->data = (MAT_INNER_TYPE*)calloc(rows * cols, sizeof(MAT_INNER_TYPE));
    if (matrix->data == NULL) {
        fprintf(stderr, "Failed to allocate matrix data!\n");
        free(matrix), matrix = NULL;
        return NULL;
    }

    return matrix;
}

void MatrixDeallocate(
    mat** matrix
) {
    assert(*matrix != NULL);

    if ((*matrix)->data != NULL)
        free((*matrix)->data), (*matrix)->data = NULL;

    free(*matrix), (*matrix) = NULL;
}

mat* MatrixRead(
    const unsigned int gridLength,
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

        // todo if rows||cols % gridLength != 0 add padding

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
                MatrixDeallocate(&matrix);
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

int MatrixPartition(
    const unsigned int gridLength,
    mat** input,
    mat*** output
) {
    assert(gridLength > 0);
    assert(input != NULL && *input != NULL);
    assert(output != NULL && *output == NULL);

    __try {
        *output = (mat**)malloc(sizeof(mat*) * gridLength * gridLength);
        if (*output == NULL) {
            fprintf(stderr, "Failed to allocate matrices array!\n");
            __throw(EXIT_FAILURE);
        }

        unsigned int rowStep = (*input)->rows / gridLength;
        unsigned int colStep = (*input)->cols / gridLength;
        if (rowStep <= 0 || colStep <= 0) {
            fprintf(stderr, "Partition failure: rowstep=%d, colStep=%d.\n", rowStep, colStep);
            __throw(EXIT_FAILURE);
        }

#ifdef CUSTOM_DEBUG
        printf("Matrix Partition: rowStep=%d, colStep=%d, gridW=%d, gridH=%d\n", rowStep, colStep, gridLength, gridLength);
#endif

        for (int i = 0; i < gridLength; ++i)
            for (int j = 0; j < gridLength; ++j) {
                int rowEnd = (i == gridLength - 1) ? (*input)->rows : (i + 1) * rowStep;
                int colEnd = (j == gridLength - 1) ? (*input)->cols : (j + 1) * colStep;
                (*output)[i * gridLength + j] = _MatrixCloneSubMatrix(
                    *input,
                    i * rowStep, j * colStep,
                    rowEnd, colEnd
                );
        }

        MatrixDeallocate(input);
    }
    __finally {
        if (__error_code) {
            if (*output != NULL) {
                for (int i = 0; i < gridLength * gridLength; ++i) {
                    if ((*output)[i] != NULL)
                        free((*output)[i]), (*output)[i] = NULL;
                }
                free(*output), *output = NULL;
            }

            return __error_code;
        }
    }

    return EXIT_SUCCESS;
}

void MatrixMultiplyAccumulate(
    mat *a,
    mat *b,
    mat *r
) {
    assert(r != NULL);
    assert(a != NULL && b != NULL);
    assert(a->cols == b->cols && a->rows == b->rows);
    assert(a->cols == r->cols && a->rows == r->rows);

    for (int i = 0; i < a->rows; i++)
        for (int k = 0; k < a->cols; k++)
            for (int j = 0; j < b->cols; j++)
            *MatrixAt(r, i, j) = *MatrixAt(r, i, j) + *MatrixAt(a, i, k) * *MatrixAt(b, k, j);
}

void MatrixFill(
    mat *matrix,
    const unsigned int iStart,
    const unsigned int jStart,
    int *data,
    const unsigned int width,
    const unsigned int height
)
{
    assert(matrix != NULL);
    assert(iStart + height <= matrix->rows && jStart + width <= matrix->cols);

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            *MatrixAt(matrix, iStart + i, jStart + j) = data[i * width + j];
}

mat* _MatrixCloneSubMatrix(
    const mat* input,
    const unsigned int rowStart,
    const unsigned int colStart,
    const unsigned int rowEnd,
    const unsigned int colEnd
) {
    assert(input != NULL);
    assert(rowStart >= 0 && colStart >= 0);
    assert(rowEnd <= input->rows && colEnd <= input->cols);
    assert(rowStart < rowEnd && colStart < colEnd);

    const unsigned int numberOfRows = rowEnd - rowStart;
    const unsigned int numberOfColumns = colEnd - colStart;
    if (numberOfRows <= 0 || numberOfColumns <= 0)
        return NULL;

    mat* matrix = MatrixAllocate(numberOfRows, numberOfColumns);
    if (matrix == NULL) {
        return NULL;
    }

    for (int i = 0; i < numberOfRows; i++) {
        for (int j = 0; j < numberOfColumns; j++)
            *MatrixAt(matrix, i, j) = *MatrixAt(input, i + rowStart, j + colStart);
    }

    return matrix;
}