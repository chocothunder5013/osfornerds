#include "stdlib.h"

int main(char* args) {
    if (!args) {
        print("Usage: cat <filename>\n");
        return 1;
    }

    int fd = open(args);
    if (fd == -1) {
        print("Error: File not found.\n");
        return 1;
    }

    char buf[64]; // Small buffer to test chunking
    int bytes;
    
    print("\n");
    while ((bytes = read(fd, buf, 63)) > 0) {
        buf[bytes] = 0; // Null terminate
        print(buf);
    }
    print("\n");

    close(fd);
    return 0;
}