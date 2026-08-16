#ifndef FS_H
#define FS_H
#include <stdint.h>
/*
 * Represents a node in the in-memory Virtual File System (VFS).
 * It can act as either a file (holding a data buffer) or a directory (holding children).
 * The filesystem is structured as a tree using 'parent', 'children', and 'next' sibling pointers.
 */
typedef struct file_node {
    char              name[32];
    char             *data;
    uint32_t          size;
    uint8_t           flags;
    struct file_node *parent;
    struct file_node *children;
    struct file_node *next;
} file_t;
extern file_t *fs_root;
void           init_fs(void *mboot_ptr);
file_t        *fs_resolve_path(const char *path);
void           fs_delete(const char *name);
#endif