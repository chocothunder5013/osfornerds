#include "stdlib.h"

void print_int(int n) {
    if (n == 0) { print("0"); return; }
    char buf[12];
    int i = 10;
    buf[11] = 0;
    while (n > 0) {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }
    print(&buf[i + 1]);
}

#include "stdlib.h"

int main() {
    time_t t;
    get_time(&t);
    
    print("\nCurrent System Time:\n");
    
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
    
    print("\n");
    return 0;
}