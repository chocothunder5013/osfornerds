#ifndef STDLIB_H
#define STDLIB_H
#include <stdint.h>

/*
 * Standard Library Header
 *
 * Provides function prototypes and structure definitions for userland applications.
 * These functions wrap system calls to interact with the kernel.
 */

// Memory management functions.
void *malloc(int size);
void  free(void *ptr);
void *sbrk(int incr);

// File system functions.
int   open(const char *filename);
void  close(int fd);
int   read(int fd, char *buf, int size);
int   write(int fd, char *buf, int size);
int   seek(int fd, int offset, int whence);
int   readdir(int index, char *buf);
void  unlink(const char *filename);

// I/O and display functions.
void  print(const char *msg);
void  print_int(int n);
void  print_hex(unsigned int n);
void  printf(const char *fmt, ...);
char  get_char();
void  clear_screen();

// Process management and control functions.
void  yield();
void  exit(int code);
int   kill(int pid);

// String and memory operations.
int   strlen(const char *str);
void *memset(void *ptr, int value, int num);
void *memcpy(void *dest, const void *src, int num);

// System information structures and functions.
typedef struct {
    uint32_t uptime_ticks;
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    uint32_t process_count;
} sysinfo_t;
int get_sysinfo(sysinfo_t *info);

// Time structures and functions.
typedef struct {
    uint8_t  second;
    uint8_t  minute;
    uint8_t  hour;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
} time_t;
void get_time(time_t *t);

// Window management structures and functions.
typedef struct {
    int      win_id;
    int      x, y, w, h;
    uint32_t color;
} rect_t;
void draw_rect(rect_t *r);
void set_cursor(int x, int y);
int  create_window(const char *title, int x, int y, int w, int h);
void win_print(int win_id, const char *msg);
void win_set_cursor(int win_id, int x, int y);
void win_set_text_color(int win_id, uint32_t color);
#endif
