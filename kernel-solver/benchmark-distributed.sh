#!/bin/bash

EXEC="main"
MAX_DIFF=0.01
INPUT_FILES=("dataset/10x10.txt" "dataset/100x100.txt" "dataset/1000x1000.txt" "dataset/10000x10000.txt")

SOURCES_DISTRIBUTED=("distributed/Grid.cpp" "distributed/main.cpp" "distributed/MPIContext.cpp" "distributed/Parser.cpp" "distributed/Worker.cpp")
RESULT_FILE_DISTRIBUTED="results-distributed.csv"

echo "Compiling distributed implementation..."
mpic++ -O3 ${SOURCES_DISTRIBUTED[@]} -o distributed/$EXEC -Wall -Werror -lm

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
            
            RAW_OUTPUT=$(mpiexec --use-hwthread-cpus --oversubscribe -n $p distributed/$EXEC $data $MAX_DIFF)
            echo "$RAW_OUTPUT"
            
            EXEC_TIME=$(echo "$RAW_OUTPUT" | grep "Execution time" | grep -oP '\d+\.\d+')
            if [ -z "$EXEC_TIME" ]; then
                echo "[ERROR] Failed to grep execution time"
                echo "$epoch,$p,$data,err" >> $RESULT_FILE_DISTRIBUTED
            else
                echo "$epoch,$p,$data,$EXEC_TIME" >> $RESULT_FILE_DISTRIBUTED
            fi

            sleep 5
        done
    done
done