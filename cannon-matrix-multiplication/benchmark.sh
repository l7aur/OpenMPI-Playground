#!/bin/bash

SOURCES=("main.c" "master.c" "slave.c" "util.c" "mat.c")
EXEC="main"
INPUT_FILES_A=("dataset/A10.txt" "dataset/A20.txt" "dataset/A48.txt" "dataset/A50.txt")
INPUT_FILES_B=("dataset/B10.txt" "dataset/B20.txt" "dataset/B48.txt" "dataset/B50.txt")
RESULT_FILE="results.csv"

echo "Compiling..."
mpicc -O3 ${SOURCES[@]} -o $EXEC -Wall -Werror

if [ $? -ne 0 ]; then
    echo "Failed to compile! Exiting..."
    exit 1
fi

echo "Compilation succeeded!"

echo "epoch,number_of_processes,input_file_a,input_file_b,execution_time" > $RESULT_FILE

echo "Starting benchmarking..."

for epoch in {1..25}; do
    echo "Epoch $epoch"

    for p in 1 4 9 16; do
        for ((i=0; i<${#INPUT_FILES_A[@]}; i++)); do
            file_a="${INPUT_FILES_A[$i]}"
            file_b="${INPUT_FILES_B[$i]}"
            if [ ! -f "$file_a" ]; then
                echo "[WARN] $file_a does not exist! Skipping..."
                continue
            fi

            if [ ! -f "$file_b" ]; then
                echo "[WARN] $file_b does not exist! Skipping..."
                continue
            fi

            echo "Testing with matrices: $file_a, $file_b and $p processes"

            RAW_OUTPUT=$(mpiexec --use-hwthread-cpus -n $p $EXEC $file_a $file_b)
            echo "$RAW_OUTPUT"

            EXEC_TIME=$(echo "$RAW_OUTPUT" | grep "\[0\] Execution time" | grep -oP '\d+\.\d+')

            if [ -z "$EXEC_TIME" ]; then
                echo "[ERROR] Failed to grep execution time"
                echo "$epoch,$p,$file_a,$file_b,err" >> $RESULT_FILE
            else
                echo "$epoch,$p,$file_a,$file_b,$EXEC_TIME" >> $RESULT_FILE
            fi

            sleep 3

        done
    done
done
