#!/bin/bash

# Check if version argument is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <version_number>"
    exit 1
fi

VERSION=$1
PROGRAM="./target/lsal_${VERSION}"
OUTPUT_FILE="results_${VERSION}.csv"

# Define the pairs of N and M (Your input sizes)
PARAMS=(
    "32 32"
    "32 65536"
    "256 65536"
    "256 300000"
)

echo "Starting test runs for Version: $VERSION"
echo "Results will be saved to: $OUTPUT_FILE"
echo "========================================"

# 1. CLEAN AND INITIALIZE THE CSV
# Using a single '>' overwrites the file completely if it already exists.
# This guarantees a fresh start for every execution.
echo "InputSize_N,InputSize_M,ExecTime_ms" > "$OUTPUT_FILE"

for param in "${PARAMS[@]}"; do
    read -r N M <<< "$param"
    echo -n "Running with N=$N, M=$M... "

    # 2. Execute the program and capture the raw output
    RAW_OUTPUT=$($PROGRAM $N $M)

    # 3. Clean the output to extract ONLY the execution time.
    EXEC_TIME=$(echo "$RAW_OUTPUT" | grep -oE '[0-9]+\.?[0-9]*' | tail -1)

    # 4. Append the clean data to the CSV
    # We use '>>' here so it adds to the file without overwriting the header!
    echo "$N,$M,$EXEC_TIME" >> "$OUTPUT_FILE"

    echo "Done! Time: $EXEC_TIME"
done

echo "========================================"
echo "All runs completed! Fresh data saved in $OUTPUT_FILE"