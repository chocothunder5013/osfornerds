#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h> // Ensure types are available

void* malloc(int size);
void free(void* ptr);
int open(const char* filename);
void close(int fd);
int read(int fd, char* buf, int size);
int write(int fd, char* buf, int size); // Add write
int seek(int fd, int offset, int whence); // Add seek
int readdir(int index, char* buf);
void unlink(const char* filename); // Add unlink (delete)
void print_int(int n);

void print(const char* msg);
void printf(const char* fmt, ...); // Add printf prototype (we'll implement a dummy one or use print)
void yield();
char get_char();
void exit(int code);
int strlen(const char* str);
void clear_screen(); // Add clear_screen

// Time struct
typedef struct {
    uint8_t second; uint8_t minute; uint8_t hour;
    uint8_t day;    uint8_t month;  uint16_t year;
} time_t;

void get_time(time_t* t);

#endif