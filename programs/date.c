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