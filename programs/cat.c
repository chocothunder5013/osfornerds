#include "stdlib.h"

/*
 * cat program
 * 
 * Reads a file and prints its contents to standard output.
 * Uses system calls to interact with the file system.
 */

// Entry point for the cat program.
int main(char *args) {
    // Check if a filename was provided as an argument.
    if (!args) {
        print("Usage: cat <filename>
");
        return 1;
    }
    
    // Attempt to open the specified file.
    int fd = open(args);
    if (fd == -1) {
        print("Error: File not found.
");
        return 1;
    }
    
    char buf[64];
    int  bytes;
    print("
");
    
    // Read and print the file in chunks of 63 bytes.
    // We reserve one byte to null-terminate the buffer for printing.
    while ((bytes = read(fd, buf, 63)) > 0) {
        buf[bytes] = 0;
        print(buf);
    }
    print("
");
    
    // Close the file descriptor to free system resources before exiting.
    close(fd);
    return 0;
}
