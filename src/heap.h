#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
uint32_t heap_used(void);
uint32_t heap_size(void);

#endif
