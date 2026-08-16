#include "stdlib.h"

/*
 * echo program
 * 
 * Prints the provided arguments to standard output.
 */

// Entry point for the echo program.
int main(int argc, char **argv) {
    // Iterate through all arguments, skipping the program name at index 0.
    for (int i = 1; i < argc; i++) {
        print(argv[i]);
        print(" ");
    }
    
    // Append a newline character after printing all arguments.
    print("
");
    return 0;
}
