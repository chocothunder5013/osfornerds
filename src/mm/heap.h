/*
 * Kernel Heap Manager API
 *
 * Provides the interface for dynamic memory allocation within the kernel.
 */
#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
#include <stddef.h>

void  init_heap();
void *kmalloc(size_t size);
void  kfree(void *ptr);

#endif