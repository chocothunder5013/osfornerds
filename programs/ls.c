#include "stdlib.h"

int main() {
    char name[32];
    int idx = 0;

    print("\n--- Files ---\n"); // printf -> print
    while (readdir(idx, name) == 1) {
        print(" - ");
        print(name);
        print("\n");
        idx++;
    }
    print("-------------\n");
    return 0;
}