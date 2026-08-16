#include "stdlib.h"

/*
 * ls program
 * 
 * Lists the files in the current directory by repeatedly calling readdir()
 * until there are no more entries.
 */

// Entry point for the ls program.
int main() {
    char name[32];
    int  idx = 0;
    
    print("
--- Files ---
");
    
    // Iterate through directory entries. The readdir() system call returns 1 
    // when an entry is successfully read and 0 or an error code otherwise.
    while (readdir(idx, name) == 1) {
        print(" - ");
        print(name);
        print("
");
        idx++;
    }
    
    print("-------------
");
    return 0;
}
