#include "stdlib.h"

/*
 * hello program
 * 
 * A simple interactive runtime test that continuously reads keyboard
 * input and prints it back to the screen until the user types 'q'.
 */

// Entry point for the hello program.
int main() {
    print("
==============================
");
    print("      The Runtime Works!      
");
    print("==============================
");
    print("Type something (q to quit):
");
    
    // Loop continuously to read user input.
    while (1) {
        char c = get_char();
        
        // Exit the loop if the user types 'q'.
        if (c == 'q') {
            break;
        }
        
        // Print the typed character back to the screen.
        if (c != 0) {
            char temp[2] = {c, 0};
            print(temp);
        }
    }
    
    print("
Goodbye from User Space!
");
    return 0;
}
