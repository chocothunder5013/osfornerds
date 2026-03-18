#!/bin/bash

# Ensure we fail if a command fails
set -e

echo "Starting code formatting..."

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format could not be found. Please run: sudo apt install clang-format"
    exit 1
fi

# Find all .c and .h files in src and programs, then pass them to clang-format
# The -i flag tells it to modify the files in-place
find src programs -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

echo "Formatting complete! (Assembly and Linker files were safely bypassed)."
