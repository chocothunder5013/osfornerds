/* programs/stdlib.c */
#include "stdlib.h"
#include <stdint.h>
#include <stdarg.h> // GCC builtin for varargs

// --- System Call Wrappers ---

// 0: PRINT
void print(const char* msg) {
	__asm__ volatile ("int $0x80" : : "a" (0), "b" (msg)); 
}

// 1: YIELD
void yield() {
	__asm__ volatile ("int $0x80" : : "a" (1)); 
}

// 2: READ CHAR
char get_char() {
	char c;
	__asm__ volatile ("int $0x80" : "=a" (c) : "a" (2)); 
	return c;
}

// 3: EXIT
void exit(int code) {
	__asm__ volatile ("int $0x80" : : "a" (3), "b"(code)); 
	while(1);
}

// 5: OPEN
int open(const char* filename) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(5), "b"(filename));
	return ret;
}

// 6: CLOSE
void close(int fd) {
	__asm__ volatile ("int $0x80" : : "a"(6), "b"(fd));
}

// 7: READ
int read(int fd, char* buf, int size) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(7), "b"(fd), "c"(buf), "d"(size));
	return ret;
}

// 8: READDIR
int readdir(int index, char* buf) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(8), "b"(index), "c"(buf));
	return ret;
}

// 9: SBRK
void* sbrk(int incr) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(9), "b"(incr));
	return (void*)ret;
}

// 10: TIME
void get_time(time_t* t) {
	__asm__ volatile ("int $0x80" : : "a"(10), "b"(t));
}

// 11: WRITE
int write(int fd, char* buf, int size) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(11), "b"(fd), "c"(buf), "d"(size));
	return ret;
}

// 12: SEEK
int seek(int fd, int offset, int whence) {
	int ret;
	__asm__ volatile ("int $0x80" : "=a"(ret) : "a"(12), "b"(fd), "c"(offset), "d"(whence));
	return ret;
}

// 13: CLEAR SCREEN
void clear_screen() {
	__asm__ volatile ("int $0x80" : : "a"(13));
}

// 14: UNLINK
void unlink(const char* filename) {
	__asm__ volatile ("int $0x80" : : "a"(14), "b"(filename)); 
}

// --- Utils & String Functions ---

int strlen(const char* str) {
	int len = 0;
	while(str[len]) len++;
	return len;
}

void* memset(void* ptr, int value, int num) {
	unsigned char* p = (unsigned char*)ptr;
	while(num--) *p++ = (unsigned char)value;
	return ptr;
}

void* memcpy(void* dest, const void* src, int num) {
	char* d = (char*)dest;
	const char* s = (const char*)src;
	while(num--) *d++ = *s++;
	return dest;
}

// --- Printf Implementation ---

void print_int(int n) {
	if (n == 0) { print("0"); return; }
	if (n < 0) { print("-"); n = -n; }

	char buffer[12];
	int i = 10;
	buffer[11] = 0;

	while (n > 0) {
		buffer[i--] = (n % 10) + '0';
		n /= 10;
	}
	print(&buffer[i + 1]);
}

void print_hex(unsigned int n) {
	char hex_chars[] = "0123456789ABCDEF";
	char buffer[11];
	buffer[0] = '0'; buffer[1] = 'x'; buffer[10] = 0;
	for(int i=0; i<8; i++) {
		buffer[9-i] = hex_chars[n & 0xF];
		n >>= 4;
	}
	print(buffer);
}

void printf(const char* fmt, ...) {
	__builtin_va_list args;
	__builtin_va_start(args, fmt);

	for (int i = 0; fmt[i] != 0; i++) {
		if (fmt[i] == '%') {
			i++;
			if (fmt[i] == 's') {
				char* s = __builtin_va_arg(args, char*);
				print(s ? s : "(null)");
			}
			else if (fmt[i] == 'd') {
				int d = __builtin_va_arg(args, int);
				print_int(d);
			}
			else if (fmt[i] == 'x') {
				unsigned int x = __builtin_va_arg(args, unsigned int);
				print_hex(x);
			}
			else if (fmt[i] == 'c') {
				char c = (char)__builtin_va_arg(args, int);
				char temp[2] = {c, 0};
				print(temp);
			}
			else {
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

// --- Malloc (Simple Linked List) ---

typedef struct block_meta {
	int size;
	struct block_meta* next;
	int free;
	int magic;
} block_meta_t;

#define META_SIZE sizeof(block_meta_t)
void* global_base = 0;

block_meta_t* find_free_block(block_meta_t** last, int size) {
	block_meta_t* current = global_base;
	while (current && !(current->free && current->size >= size)) {
		*last = current;
		current = current->next;
	}
	return current;
}

block_meta_t* request_space(block_meta_t* last, int size) {
	block_meta_t* block = (block_meta_t*)sbrk(0);
	void* request = sbrk(size + META_SIZE);

	if (request == (void*)-1) return 0;

	if (last) last->next = block;
	block->size = size;
	block->next = 0;
	block->free = 0;
	block->magic = 0x12345678;
	return block;
}

void* malloc(int size) {
	if (size <= 0) return 0;
	block_meta_t* block;

	if (!global_base) {
		block = request_space(0, size);
		if (!block) return 0;
		global_base = block;
	} else {
		block_meta_t* last = global_base;
		block = find_free_block(&last, size);
		if (!block) {
			block = request_space(last, size);
			if (!block) return 0;
		} else {
			block->free = 0;
			block->magic = 0x77777777;
		}
	}
	return (void*)(block + 1);
}

void free(void* ptr) {
	if (!ptr) return;
	block_meta_t* block = (block_meta_t*)ptr - 1;
	if (block->magic == 0x12345678 || block->magic == 0x77777777) {
		block->free = 1;
	}
}
