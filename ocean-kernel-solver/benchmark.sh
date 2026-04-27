#!/bin/bash

SOURCES=("main.c" "master.c" "slave.c" "util.c" "mat.c")
EXEC="main"
INPUT_FILES_A=()
INPUT_FILES_B=()
RESULT_FILE="results.csv"

echo "Compiling..."
mpicc -O3 ${SOURCES[@]} -o $EXEC -Wall -Werror

if [ $? -ne 0 ]; then
    echo "Failed to compile! Exiting..."
    exit 1
fi

echo "Compilation succeeded!"

# echo "epoch,number_of_processes,input_file_a,input_file_b,execution_time" > $RESULT_FILE

echo "Starting benchmarking..."