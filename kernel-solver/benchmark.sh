#!/bin/bash

EXEC="main"
MAX_DIFF=0.2
INPUT_FILES=("dataset/10x10.txt" "dataset/100x100.txt" "dataset/1000x1000.txt" "dataset/10000x10000.txt")

SOURCES_SHARED_MEMORY=("shared-memory/main.cpp" "shared-memory/Grid.cpp" "common/Parser.cpp")
RESULT_FILE_SHARED_MEMORY="results-shared-memory.csv"

echo "Compiling shared memory implementation..."
g++ -O3 ${SOURCES_SHARED_MEMORY[@]} -o shared-memory/$EXEC -Wall -Werror -std=c++20

if [ $? -ne 0 ]; then
    echo "Failed to compile shared memory implementation! Exiting..."
    exit 1
fi

echo "epoch,number_of_threads,input_file,execution_time" > $RESULT_FILE_SHARED_MEMORY
echo "Starting benchmarking for shared memory implementation..."

for epoch in {1..25}; do
    echo "Epoch $epoch"

    for data in "${INPUT_FILES[@]}"; do
        if [ ! -f "$data" ]; then
            echo "Warning: $data does not exist! Skipping..."
            continue
        fi

        echo "Testing with dataset: $data"

        for t in 1 2 4 8 16 32 64; do
            echo "Running with $t thread(s)..."
            
            RAW_OUTPUT=$(./shared-memory/$EXEC $data $MAX_DIFF $t)
            echo "$RAW_OUTPUT"
            
            EXEC_TIME=$(echo "$RAW_OUTPUT" | grep "Execution time" | grep -oP '\d+\.\d+')
            if [ -z "$EXEC_TIME" ]; then
                echo "[ERROR] Failed to grep execution time"
                echo "$epoch,$t,$data,err" >> $RESULT_FILE_SHARED_MEMORY
            else
                echo "$epoch,$t,$data,$EXEC_TIME" >> $RESULT_FILE_SHARED_MEMORY
            fi

            sleep 3
        done
    done
done

SOURCES_DISTRIBUTED=()
RESULT_FILE_DISTRIBUTED="results-distributed.csv"

echo "Compiling distributed implementation..."
mpicc -O3 ${SOURCES_DISTRIBUTED[@]} -o distributed/$EXEC -Wall -Werror

if [ $? -ne 0 ]; then
    echo "Failed to compile distributed implementation! Exiting..."
    exit 1
fi

echo "Compilation succeeded for distributed implementation!"

echo "epoch,number_of_processes,input_file,execution_time" > $RESULT_FILE_DISTRIBUTED
echo "Starting benchmarking for distributed implementation..."

for epoch in {1..25}; do
    echo "Epoch $epoch"

    for data in "${INPUT_FILES[@]}"; do
        if [ ! -f "$data" ]; then
            echo "Warning: $data does not exist! Skipping..."
            continue
        fi

        echo "Testing with dataset: $data"

        for p in 1 2 4 8 16 32 64; do
            echo "Running with $p process(es)..."
            
            RAW_OUTPUT=$(mpiexec --use-hwthread-cpus -n $p distributed/$EXEC)
            echo "$RAW_OUTPUT"
            
            EXEC_TIME=$(echo "$RAW_OUTPUT" | grep "Execution time" | grep -oP '\d+\.\d+')
            if [ -z "$EXEC_TIME" ]; then
                echo "[ERROR] Failed to grep execution time"
                echo "$epoch,$t,$data,err" >> $RESULT_FILE_DISTRIBUTED
            else
                echo "$epoch,$t,$data,$EXEC_TIME" >> $RESULT_FILE_DISTRIBUTED
            fi

            sleep 3
        done
    done
done