#!/bin/bash

# Ensure the user provided arguments
if [ $# -eq 0 ]; then
    echo "Usage: $0 <N> <M> [additional arguments...]"
    echo "Example: $0 32 65536"
    exit 1
fi

TARGET_DIR="target"
CSV_FILE="benchmark_results.csv"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found. Please run 'make' first."
    exit 1
fi

# Initialize the CSV file with headers
echo "Program,Average_Time_Seconds" > "$CSV_FILE"

echo "Starting benchmarks... (This might take a while)"
echo "Results will be saved to: $CSV_FILE"
echo "========================================"

# Loop through everything in the target directory
for program in "$TARGET_DIR"/*; do
    
    if [ -f "$program" ] && [ -x "$program" ]; then
        prog_name=$(basename "$program")
        echo -n "Benchmarking $prog_name "
        
        times=()
        
        # Run the program 12 times
        for i in {1..12}; do
            
            # Run the program and capture ALL its output
            output=$("$program" "$@")
            
            # Extract just the line with "execution time", then grab the number
            elapsed=$(echo "$output" | grep "execution time" | awk '{print $8}')
            
            times+=("$elapsed")
            
            echo -n "."
        done        
        echo 

        # Sort times, drop min/max, and calculate average
        # - printf outputs the array one per line
        # - sort -n sorts them numerically from smallest to largest
        # - awk skips the 1st (NR>1) and 12th (NR<12) rows, sums the rest, and divides by 10
        avg=$(printf "%s\n" "${times[@]}" | sort -n | awk '
            NR>1 && NR<12 { sum += $1; count++ } 
            END { printf "%.6f", sum/count }
        ')
        
        echo "  -> Average Time: $avg seconds"
        
        # 6. Append the result to the CSV file
        echo "$prog_name,$avg" >> "$CSV_FILE"
    fi
done

echo "========================================"
echo "All done! Data has been written to $CSV_FILE"