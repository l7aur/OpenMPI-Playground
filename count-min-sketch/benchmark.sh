#!/bin/bash

SOURCES=("main.c" "master.c" "slave.c" "util.c")
EXEC="main"
NUMBER_OF_HASH_FUNCTIONS=5
NUMBER_OF_COUNTERS=10
INPUT_FILES=("dataset/data1000.txt" "dataset/data2000.txt" "dataset/data300000.txt")
RESULT_FILE="results.csv"

echo "Compiling..."
mpicc -O3 ${SOURCES[@]} -o $EXEC

if [ $? -ne 0 ]; then
    echo "Failed to compile! Exiting..."
    exit 1
fi
echo "Compilation succeeded!"

echo "epoch,number_of_processes,input_file,execution_time" > $RESULT_FILE

echo "Starting benchmarking..."

for epoch in {1..50}; do
    echo "Epoch $epoch"

    for data in "${INPUT_FILES[@]}"; do
        if [ ! -f "$data" ]; then
            echo "[WARN] $data does not exist! Skipping..."
            continue
        fi

        echo "Testing with dataset: $data"

        for p in {1..16}; do
            echo "Running with -n $p..."

            RAW_OUTPUT=$(mpiexec --use-hwthread-cpus -n $p $EXEC $NUMBER_OF_HASH_FUNCTIONS $NUMBER_OF_COUNTERS $data)
            echo "$RAW_OUTPUT"

            EXEC_TIME=$(echo "$RAW_OUTPUT" | grep "\[0\] Execution time" | grep -oP '\d+\.\d+')
            if [ -z "$EXEC_TIME" ]; then
                echo "[ERROR] Failed to grep execution time"
                echo "$epoch,$p,$data,err" >> $RESULT_FILE
            else
                echo "$epoch,$p,$data,$EXEC_TIME" >> $RESULT_FILE
            fi

            sleep 3

        done
    done
done