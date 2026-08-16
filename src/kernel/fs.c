#include "multiboot.h"
#include <stdint.h>
#include "../mm/heap.h"
#include "../drivers/ata.h"
#include "../kernel/process.h"
extern void       term_print(const char *str);
extern int        strcmp(const char *s1, const char *s2);
extern int        strlen(const char *str);
extern void       strcpy_safe(char *dest, const char *src);
extern process_t *current_process;
extern void       serial_log(char *str);
#define FS_FILE 0
#define FS_DIRECTORY 1
#define FS_MAGIC 0xDEADC0DE
#define MAX_FILES 64
#define DATA_START_SECTOR 10
typedef struct file_node {
    char              name[32];
    char             *data;
    uint32_t          size;
    uint8_t           flags;
    struct file_node *parent;
    struct file_node *children;
    struct file_node *next;
} file_t;
typedef struct {
    char     name[64];
    uint32_t size;
    uint32_t data_sector_offset;
} disk_file_entry_t;
void    fs_save_to_disk();
file_t *fs_root = 0;
file_t *fs_create_node(const char *name, int flags) {
    file_t *new_node = (file_t *)kmalloc(sizeof(file_t));
    for (int i = 0; i < 32; i++)
        new_node->name[i] = 0;
    int i = 0;
    while (name[i] && i < 31) {
        new_node->name[i] = name[i];
        i++;
    }
    new_node->flags    = flags;
    new_node->data     = 0;
    new_node->size     = 0;
    new_node->parent   = 0;
    new_node->children = 0;
    new_node->next     = 0;
    return new_node;
}
void fs_insert_child(file_t *parent, file_t *child) {
    if (!parent || parent->flags != FS_DIRECTORY)
        return;
    child->parent    = parent;
    child->next      = parent->children;
    parent->children = child;
}
file_t *fs_find_child(file_t *parent, const char *name) {
    if (!parent || parent->flags != FS_DIRECTORY)
        return 0;
    file_t *curr = parent->children;
    while (curr) {
        if (strcmp(curr->name, name) == 0)
            return curr;
        curr = curr->next;
    }
    return 0;
}
/*
 * Traverses the file system tree to locate a file or directory based on the given path.
 * Handles both absolute (starting with '/') and relative paths.
 * Resolves special directory references like '.' and '..'.
 */
file_t *fs_resolve_path(const char *path) {
    if (!path || !path[0])
        return current_process ? current_process->cwd : fs_root;
    file_t *current;
    if (path[0] == '/') {
        current = fs_root;
    } else {
        current = current_process ? current_process->cwd : fs_root;
    }
    char buffer[32];
    int  path_idx = (path[0] == '/') ? 1 : 0;
    while (path[path_idx]) {
        int buf_idx = 0;
        while (path[path_idx] != '/' && path[path_idx] != 0) {
            if (buf_idx < 31) {
                buffer[buf_idx++] = path[path_idx];
            }
            path_idx++;
        }
        buffer[buf_idx] = 0;
        if (buf_idx > 0) {
            if (strcmp(buffer, ".") == 0) {
            } else if (strcmp(buffer, "..") == 0) {
                if (current->parent)
                    current = current->parent;
            } else {
                file_t *next = fs_find_child(current, buffer);
                if (!next)
                    return 0;
                current = next;
            }
        }
        if (path[path_idx] == '/')
            path_idx++;
    }
    return current;
}
void fs_mkdir(const char *name) {
    if (fs_find_child(current_process->cwd, name)) {
        term_print("Error: Directory already exists.\n");
        return;
    }
    file_t *dir = fs_create_node(name, FS_DIRECTORY);
    fs_insert_child(current_process->cwd, dir);
    term_print("Directory created.\n");
}
void fs_touch(const char *name) {
    if (fs_find_child(current_process->cwd, name)) {
        term_print("Error: File already exists.\n");
        return;
    }
    file_t *f = fs_create_node(name, FS_FILE);
    fs_insert_child(current_process->cwd, f);
    term_print("File created.\n");
}
void fs_list(const char *path) {
    file_t *dir;
    if (path)
        dir = fs_resolve_path(path);
    else
        dir = current_process->cwd;
    if (!dir || dir->flags != FS_DIRECTORY) {
        term_print("Invalid directory.\n");
        return;
    }
    term_print("\n--- Listing ---\n");
    file_t *curr = dir->children;
    while (curr) {
        term_print(curr->name);
        if (curr->flags == FS_DIRECTORY)
            term_print("/");
        term_print("\n");
        curr = curr->next;
    }
    term_print("---------------\n");
}
void fs_cat(const char *path) {
    file_t *f = fs_resolve_path(path);
    if (!f || f->flags == FS_DIRECTORY) {
        term_print("File not found or is a directory.\n");
        return;
    }
    if (!f->data) {
        term_print("(Empty)\n");
        return;
    }
    term_print("\n");
    for (uint32_t i = 0; i < f->size; i++) {
        char t[2] = {f->data[i], 0};
        term_print(t);
    }
    term_print("\n");
}
void fs_write(const char *path, const char *content) {
    file_t *f = fs_resolve_path(path);
    if (!f) {
        term_print("File not found.\n");
        return;
    }
    if (f->data)
        kfree(f->data);
    int len = strlen(content);
    f->data = (char *)kmalloc(len + 1);
    for (int i = 0; i < len; i++)
        f->data[i] = content[i];
    f->size = len;
    term_print("Written.\n");
}
void fs_delete(const char *name) {
    file_t *parent = current_process->cwd;
    file_t *curr   = parent->children;
    file_t *prev   = 0;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            if (curr->flags == FS_DIRECTORY && curr->children) {
                term_print("Error: Directory not empty.\n");
                return;
            }
            if (curr->data)
                kfree(curr->data);
            if (prev)
                prev->next = curr->next;
            else
                parent->children = curr->next;
            kfree(curr);
            term_print("Deleted.\n");
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    term_print("Not found.\n");
}
int get_free_fd() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (current_process->fd_table[i].file_node == 0)
            return i;
    }
    return -1;
}
int sys_open(const char *path) {
    file_t *f = fs_resolve_path(path);
    if (!f)
        return -1;
    int fd = get_free_fd();
    if (fd == -1)
        return -1;
    current_process->fd_table[fd].file_node = f;
    current_process->fd_table[fd].offset    = 0;
    return fd;
}
void sys_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        current_process->fd_table[fd].file_node = 0;
    }
}
int sys_read_file(int fd, char *buffer, int size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES)
        return -1;
    file_descriptor_t *desc = &current_process->fd_table[fd];
    file_t            *file = (file_t *)desc->file_node;
    if (!file)
        return -1;
    int left = file->size - desc->offset;
    if (left <= 0)
        return 0;
    int to_read = (size < left) ? size : left;
    for (int i = 0; i < to_read; i++) {
        buffer[i] = file->data[desc->offset + i];
    }
    desc->offset += to_read;
    return to_read;
}
int sys_write_file(int fd, char *buffer, int size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES)
        return -1;
    file_descriptor_t *desc = &current_process->fd_table[fd];
    file_t            *file = (file_t *)desc->file_node;
    if (!file)
        return -1;
    int end_pos = desc->offset + size;
    if (end_pos > (int)file->size) {
        char *new_data = (char *)kmalloc(end_pos);
        for (uint32_t i = 0; i < file->size; i++)
            new_data[i] = file->data[i];
        if (file->data)
            kfree(file->data);
        file->data = new_data;
        file->size = end_pos;
    }
    for (int i = 0; i < size; i++) {
        file->data[desc->offset + i] = buffer[i];
    }
    desc->offset += size;
    fs_save_to_disk();
    return size;
}
int sys_seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES)
        return -1;
    file_descriptor_t *desc = &current_process->fd_table[fd];
    file_t            *file = (file_t *)desc->file_node;
    if (!file)
        return -1;
    int new_pos = desc->offset;
    if (whence == 0)
        new_pos = offset;
    if (whence == 1)
        new_pos += offset;
    if (whence == 2)
        new_pos = file->size + offset;
    if (new_pos < 0)
        new_pos = 0;
    if (new_pos > (int)file->size)
        new_pos = file->size;
    desc->offset = new_pos;
    return new_pos;
}
int sys_chdir(const char *path) {
    file_t *dir = fs_resolve_path(path);
    if (!dir || dir->flags != FS_DIRECTORY)
        return -1;
    current_process->cwd = dir;
    return 0;
}
void sys_getcwd(char *buf, int size) {
    (void)size;
    if (current_process->cwd == fs_root) {
        strcpy_safe(buf, "/");
    } else {
        buf[0] = '/';
        strcpy_safe(buf + 1, current_process->cwd->name);
    }
}
int sys_readdir(int index, char *buf) {
    file_t *curr = current_process->cwd->children;
    int     i    = 0;
    while (curr) {
        if (i == index) {
            strcpy_safe(buf, curr->name);
            return 1;
        }
        curr = curr->next;
        i++;
    }
    return 0;
}
void path_join(char *dest, const char *parent, const char *child) {
    int i = 0;
    while (parent[i]) {
        dest[i] = parent[i];
        i++;
    }
    if (i > 0 && parent[i - 1] != '/')
        dest[i++] = '/';
    int j = 0;
    while (child[j]) {
        dest[i++] = child[j++];
    }
    dest[i] = 0;
}
void fs_flatten_tree(file_t *node, char *current_path, disk_file_entry_t *table, int *idx,
                     int *sector_offset) {
    if (!node || *idx >= MAX_FILES)
        return;
    char full_path[64];
    path_join(full_path, current_path, node->name);
    if (node->flags == FS_FILE) {
        strcpy_safe(table[*idx].name, full_path);
        table[*idx].size               = node->size;
        table[*idx].data_sector_offset = *sector_offset;
        int sectors                    = (node->size + 511) / 512;
        if (node->data) {
            uint32_t *buf = (uint32_t *)kmalloc(sectors * 512);
            for (int k = 0; k < sectors * 512; k++)
                ((char *)buf)[k] = 0;
            for (uint32_t k = 0; k < node->size; k++)
                ((char *)buf)[k] = node->data[k];
            ata_write_sectors(DATA_START_SECTOR + *sector_offset, sectors, buf);
            kfree(buf);
        }
        *sector_offset += sectors;
        (*idx)++;
    }
    if (node->flags == FS_DIRECTORY) {
        fs_flatten_tree(node->children, full_path, table, idx, sector_offset);
    }
    fs_flatten_tree(node->next, current_path, table, idx, sector_offset);
}
/*
 * Serializes the file system structure and file contents, then writes them to disk sectors.
 * It first flattens the file tree into a static table of disk_file_entry_t records,
 * writes the MAGIC header and table, and finally writes the file data clusters.
 */
void fs_save_to_disk() {
    term_print(" [FS] Saving...\n");
    uint32_t *header         = (uint32_t *)kmalloc(512);
    header[0]                = FS_MAGIC;
    disk_file_entry_t *table = (disk_file_entry_t *)kmalloc(MAX_FILES * sizeof(disk_file_entry_t));
    for (uint32_t i = 0; i < MAX_FILES * sizeof(disk_file_entry_t); i++)
        ((char *)table)[i] = 0;
    int count  = 0;
    int offset = 0;
    if (fs_root->children) {
        fs_flatten_tree(fs_root->children, "", table, &count, &offset);
    }
    header[1] = count;
    ata_write_sectors(0, 1, header);
    ata_write_sectors(1, 8, (uint32_t *)table);
    kfree(header);
    kfree(table);
    term_print(" [FS] Saved.\n");
}
/*
 * Reads the filesystem index and file data from the ATA disk.
 * It checks the boot sector for the FS_MAGIC signature. If found, it reads the
 * file table and rebuilds the in-memory tree nodes and data buffers.
 */
void fs_load_from_disk() {
    uint32_t *header = (uint32_t *)kmalloc(512);
    ata_read_sectors(0, 1, header);
    if (header[0] != FS_MAGIC) {
        kfree(header);
        serial_log(" [FS] No filesystem found.\n");
        return;
    }
    int count = header[1];
    kfree(header);
    term_print(" [FS] Loading disk data...\n");
    disk_file_entry_t *table = (disk_file_entry_t *)kmalloc(MAX_FILES * sizeof(disk_file_entry_t));
    ata_read_sectors(1, 8, (uint32_t *)table);
    for (int i = 0; i < count; i++) {
        if (table[i].name[0] == 0)
            continue;
        file_t *f         = fs_create_node(table[i].name, FS_FILE);
        f->size           = table[i].size;
        f->data           = (char *)kmalloc(f->size + 1);
        int       sectors = (f->size + 511) / 512;
        uint32_t *buf     = (uint32_t *)kmalloc(sectors * 512);
        ata_read_sectors(DATA_START_SECTOR + table[i].data_sector_offset, sectors, buf);
        for (uint32_t k = 0; k < f->size; k++)
            f->data[k] = ((char *)buf)[k];
        f->data[f->size] = 0;
        kfree(buf);
        fs_insert_child(fs_root, f);
    }
    kfree(table);
}
/*
 * Initializes the root filesystem node.
 * It parses Multiboot modules (like the initial ramdisk) to populate the root
 * directory before attempting to load any persistent filesystem state from disk.
 */
void init_fs(multiboot_info_t *mboot_ptr) {
    fs_root = fs_create_node("/", FS_DIRECTORY);
    if (mboot_ptr->flags & (1 << 3)) {
        multiboot_module_t *mod = (multiboot_module_t *)mboot_ptr->mods_addr;
        for (uint32_t i = 0; i < mboot_ptr->mods_count; i++) {
            char    *name = (char *)mod[i].string;
            file_t  *f    = fs_create_node(name, FS_FILE);
            uint32_t len  = mod[i].mod_end - mod[i].mod_start;
            f->data       = (char *)kmalloc(len + 1);
            char *src     = (char *)mod[i].mod_start;
            for (uint32_t k = 0; k < len; k++)
                f->data[k] = src[k];
            f->data[len] = 0;
            f->size      = len;
            fs_insert_child(fs_root, f);
            term_print(" [FS] Loaded module: ");
            term_print(name);
            term_print("\n");
        }
    }
    fs_load_from_disk();
}