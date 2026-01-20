#ifndef FS_H
#define FS_H

#include <stdint.h>

// Struct Definitions
typedef struct file_node {
    char name[32];
    char* data;
    uint32_t size;
    uint8_t flags;
    struct file_node* parent;
    struct file_node* children;
    struct file_node* next;
} file_t;

extern file_t* fs_root; // Extern declaration

// Function Prototypes
void init_fs(void* mboot_ptr); // Use void* to avoid circular include dep
file_t* fs_resolve_path(const char* path);
void fs_delete(const char* name);
// ... Add prototypes for fs_read, fs_write, etc.

#endif