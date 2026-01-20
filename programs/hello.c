#include "stdlib.h"

int main() {
    print("\n==============================\n");
    print("      The Runtime Works!      \n");
    print("==============================\n");
    
    print("Type something (q to quit):\n");

    while(1) {
        char c = get_char();
        if (c == 'q') {
            break; // Break loop to exit
        }
        if (c != 0) {
            char temp[2] = {c, 0};
            print(temp); // Echo back
        }
    }

    print("\nGoodbye from User Space!\n");
    return 0; // This will trigger entry.S to call exit()
}