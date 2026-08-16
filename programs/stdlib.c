#include "stdlib.h"
#include <stdint.h>
#include <stdarg.h>

/*
 * Userland Standard Library
 * 
 * This file implements the C standard library for userland applications. 
 * It wraps hardware interrupts (int 0x80) to execute system calls, 
 * abstracting the kernel interface for processes.
 */

// Issue a system call to print a string.
// EAX = 0 (sys_print), EBX = message pointer.
void print(const char *msg) {
    __asm__ volatile("int $0x80" : : "a"(0), "b"(msg) : "memory");
}

// Yield the processor to another task.
// EAX = 1 (sys_yield).
void yield() {
    __asm__ volatile("int $0x80" : : "a"(1));
}

// Read a single character from the keyboard buffer.
// EAX = 2 (sys_get_char). Returns the character in EAX.
char get_char() {
    char c;
    __asm__ volatile("int $0x80" : "=a"(c) : "a"(2));
    return c;
}

// Populate a sysinfo_t structure with system statistics.
// EAX = 17 (sys_get_sysinfo), EBX = pointer to sysinfo_t.
int get_sysinfo(sysinfo_t *info) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(17), "b"(info) : "memory");
    return ret;
}

// Move the cursor to a specific coordinate within a window.
void win_set_cursor(int win_id, int x, int y) {
    __asm__ volatile("int $0x80" : : "a"(22), "b"(win_id), "c"(x), "d"(y));
}

// Set the text color for a window.
void win_set_text_color(int win_id, uint32_t color) {
    __asm__ volatile("int $0x80" : : "a"(23), "b"(win_id), "c"(color));
}

// Terminate a process by its PID.
int kill(int pid) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(24), "b"(pid));
    return ret;
}

// Terminate the current process and return an exit code.
// EAX = 3 (sys_exit), EBX = exit code.
// Uses an infinite loop to halt execution if the syscall returns.
void exit(int code) {
    __asm__ volatile("int $0x80" : : "a"(3), "b"(code));
    while (1)
        ;
}

// Create a new window on the screen.
int create_window(const char *title, int x, int y, int w, int h) {
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(20), "b"(title), "c"(x), "d"(y), "S"(w), "D"(h)
                     : "memory");
    return ret;
}

// Print a string inside a specific window.
void win_print(int win_id, const char *msg) {
    __asm__ volatile("int $0x80" : : "a"(21), "b"(win_id), "c"(msg) : "memory");
}

// Draw a rectangle using the window manager.
void draw_rect(rect_t *r) {
    __asm__ volatile("int $0x80" : : "a"(18), "b"(r) : "memory");
}

// Set the global screen cursor position.
void set_cursor(int x, int y) {
    __asm__ volatile("int $0x80" : : "a"(19), "b"(x), "c"(y));
}

// Open a file and return a file descriptor.
int open(const char *filename) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(5), "b"(filename));
    return ret;
}

// Close an open file descriptor.
void close(int fd) {
    __asm__ volatile("int $0x80" : : "a"(6), "b"(fd));
}

// Read bytes from a file into a buffer.
int read(int fd, char *buf, int size) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(7), "b"(fd), "c"(buf), "d"(size) : "memory");
    return ret;
}

// Read a directory entry by index.
int readdir(int index, char *buf) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(8), "b"(index), "c"(buf) : "memory");
    return ret;
}

// Increase or decrease the data segment size (program break).
void *sbrk(int incr) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(9), "b"(incr));
    return (void *)ret;
}

// Populate a time_t structure with the current system time.
void get_time(time_t *t) {
    __asm__ volatile("int $0x80" : : "a"(10), "b"(t) : "memory");
}

// Write bytes from a buffer to a file.
int write(int fd, char *buf, int size) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(11), "b"(fd), "c"(buf), "d"(size));
    return ret;
}

// Change the current read/write position of a file.
int seek(int fd, int offset, int whence) {
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(12), "b"(fd), "c"(offset), "d"(whence));
    return ret;
}

// Clear the entire screen.
void clear_screen() {
    __asm__ volatile("int $0x80" : : "a"(13));
}

// Remove a file from the file system.
void unlink(const char *filename) {
    __asm__ volatile("int $0x80" : : "a"(14), "b"(filename));
}

// Calculate the length of a null-terminated string.
int strlen(const char *str) {
    int len = 0;
    while (str[len])
        len++;
    return len;
}

// Fill a block of memory with a specific value.
void *memset(void *ptr, int value, int num) {
    unsigned char *p = (unsigned char *)ptr;
    while (num--)
        *p++ = (unsigned char)value;
    return ptr;
}

// Copy a block of memory from a source to a destination.
void *memcpy(void *dest, const void *src, int num) {
    char       *d = (char *)dest;
    const char *s = (const char *)src;
    while (num--)
        *d++ = *s++;
    return dest;
}

// Print a signed integer as a string.
void print_int(int n) {
    if (n == 0) {
        print("0");
        return;
    }
    if (n < 0) {
        print("-");
        n = -n;
    }
    char buffer[12];
    int  i     = 10;
    buffer[11] = 0;
    
    // Extract digits in reverse order by taking the modulus of 10.
    while (n > 0) {
        buffer[i--] = (n % 10) + '0';
        n /= 10;
    }
    print(&buffer[i + 1]);
}

// Print an unsigned integer in hexadecimal format.
void print_hex(unsigned int n) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    buffer[0]  = '0';
    buffer[1]  = 'x';
    buffer[10] = 0;
    
    // Extract 4-bit nibbles starting from the least significant bits.
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    print(buffer);
}

// Format and print a string with variable arguments.
// Supports %s (string), %d (integer), %x (hexadecimal), and %c (character).
void printf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    for (int i = 0; fmt[i] != 0; i++) {
        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == 's') {
                char *s = __builtin_va_arg(args, char *);
                print(s ? s : "(null)");
            } else if (fmt[i] == 'd') {
                int d = __builtin_va_arg(args, int);
                print_int(d);
            } else if (fmt[i] == 'x') {
                unsigned int x = __builtin_va_arg(args, unsigned int);
                print_hex(x);
            } else if (fmt[i] == 'c') {
                char c       = (char)__builtin_va_arg(args, int);
                char temp[2] = {c, 0};
                print(temp);
            } else {
                char temp[2] = {fmt[i], 0};
                print(temp);
            }
        } else {
            char temp[2] = {fmt[i], 0};
            print(temp);
        }
    }
    __builtin_va_end(args);
}

// Memory Allocation Implementation

typedef struct block_meta {
    int                size;
    struct block_meta *next;
    int                free;
    int                magic;
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)
void         *global_base = 0;

// Locate a free block that fits the requested size using a first-fit search.
block_meta_t *find_free_block(block_meta_t **last, int size) {
    block_meta_t *current = global_base;
    while (current && !(current->free && current->size >= size)) {
        *last   = current;
        current = current->next;
    }
    return current;
}

// Extend the heap by requesting space from the kernel.
block_meta_t *request_space(block_meta_t *last, int size) {
    block_meta_t *block   = (block_meta_t *)sbrk(0);
    void         *request = sbrk(size + META_SIZE);
    if (request == (void *)-1)
        return 0;
    if (last)
        last->next = block;
    block->size  = size;
    block->next  = 0;
    block->free  = 0;
    block->magic = 0x12345678;
    return block;
}

// Allocate memory using a first-fit algorithm.
void *malloc(int size) {
    if (size <= 0)
        return 0;
    block_meta_t *block;
    
    // Initialize the heap if this is the first allocation.
    if (!global_base) {
        block = request_space(0, size);
        if (!block)
            return 0;
        global_base = block;
    } else {
        block_meta_t *last = global_base;
        block              = find_free_block(&last, size);
        
        if (!block) {
            // Expand the heap if no suitable block exists.
            block = request_space(last, size);
            if (!block)
                return 0;
        } else {
            // Re-use the existing free block.
            block->free  = 0;
            block->magic = 0x77777777;
        }
    }
    
    // Return a pointer to the usable memory region immediately following the metadata header.
    return (void *)(block + 1);
}

// Release allocated memory by marking the block as free.
void free(void *ptr) {
    if (!ptr)
        return;
    block_meta_t *block = (block_meta_t *)ptr - 1;
    
    // Verify the magic number to ensure this is a valid memory block.
    if (block->magic == 0x12345678 || block->magic == 0x77777777) {
        block->free = 1;
    }
}
