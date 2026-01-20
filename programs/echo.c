#include "stdlib.h"

int main(int argc, char** argv) {
    // Phase 6: Standard C parsing
    
    for (int i = 1; i < argc; i++) {
        print(argv[i]); // Changed printf -> print
        print(" ");
    }
    print("\n");
    return 0;
}