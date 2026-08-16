#include "stdlib.h"

/*
 * date program
 * 
 * Retrieves the current system time using a system call and prints it
 * to the screen in a formatted string.
 */

// Entry point for the date program.
int main() {
    time_t t;
    
    // Retrieve the current time from the system via the get_time() system call wrapper.
    get_time(&t);
    
    // Output the formatted time.
    print("
Current System Time:
");
    print("20");
    print_int(t.year - 2000);
    print("/");
    print_int(t.month);
    print("/");
    print_int(t.day);
    print("  ");
    print_int(t.hour);
    print(":");
    print_int(t.minute);
    print("
");
    return 0;
}
