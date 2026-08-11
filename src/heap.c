#include "heap.h"

#include <stddef.h>
#include <stdint.h>

#include "paging.h"
#include "pmm.h"

#define HEAP_START 0x40000000u
#define HEAP_MAX 0x10000000u
#define PAGE_SIZE 4096u

#define BLOCK_HEADER_SIZE 16u
#define BLOCK_ALIGN 8u
#define HEAP_MAGIC 0xA11C0DE5u

struct block {
	uint32_t magic;
	uint32_t size;
	uint32_t free;
	struct block *next;
};

static struct block *freelist = NULL;
static uint32_t heap_end = HEAP_START;
static uint32_t heap_used_bytes = 0;

static size_t round_up(size_t n, size_t a) { return (n + a - 1u) & ~(a - 1u); }

void heap_init(void) {
	freelist = NULL;
	heap_end = HEAP_START;
	heap_used_bytes = 0;
}

uint32_t heap_size(void) { return heap_end - HEAP_START; }

uint32_t heap_used(void) { return heap_used_bytes; }

static int heap_grow(size_t need) {
	uint32_t start = heap_end;
	while (heap_end - start < need) {
		if (heap_size() >= HEAP_MAX) {
			return 0;
		}
		uint32_t page = pmm_alloc_frame();
		if (page == 0u) {
			return 0;
		}
		paging_map_page(heap_end, page);
		heap_end += PAGE_SIZE;
	}
	struct block *b = (struct block *)start;
	b->magic = HEAP_MAGIC;
	b->size = heap_end - start;
	b->free = 1;
	b->next = freelist;
	freelist = b;
	return 1;
}

void *kmalloc(size_t size) {
	if (size == 0u) {
		return NULL;
	}
	size_t need = round_up(size + BLOCK_HEADER_SIZE, BLOCK_ALIGN);

	struct block *b;
	struct block **link = &freelist;
	for (b = freelist; b != NULL; b = b->next) {
		if (b->free && b->size >= need) {
			break;
		}
		link = &b->next;
	}
	if (b == NULL) {
		if (!heap_grow(need)) {
			return NULL;
		}
		link = &freelist;
		for (b = freelist; b != NULL; b = b->next) {
			if (b->free && b->size >= need) {
				break;
			}
			link = &b->next;
		}
		if (b == NULL) {
			return NULL;
		}
	}

	size_t remain = b->size - need;
	if (remain >= BLOCK_HEADER_SIZE + BLOCK_ALIGN) {
		struct block *nb = (struct block *)((uint8_t *)b + need);
		nb->magic = HEAP_MAGIC;
		nb->size = remain;
		nb->free = 1;
		nb->next = b->next;
		*link = nb;
		b->size = need;
	} else {
		*link = b->next;
	}

	b->free = 0;
	b->next = NULL;
	heap_used_bytes += b->size;
	return (uint8_t *)b + BLOCK_HEADER_SIZE;
}

void kfree(void *ptr) {
	if (ptr == NULL) {
		return;
	}
	struct block *b = (struct block *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
	if (b->magic != HEAP_MAGIC || b->free != 0u) {
		return;
	}
	b->free = 1;
	b->next = freelist;
	freelist = b;
	heap_used_bytes -= b->size;
}
