#!/bin/bash
set -e
echo "Starting code formatting..."
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format could not be found. Please run: sudo apt install clang-format"
    exit 1
fi
find src programs -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +
echo "Formatting complete! (Assembly and Linker files were safely bypassed)."