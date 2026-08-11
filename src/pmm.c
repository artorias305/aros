#include "pmm.h"

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096u
#define PAGE_SHIFT 12u

#define PMM_MAX_PHYS (1024u * 1024u * 1024u)
#define PMM_BITMAP_BITS (PMM_MAX_PHYS / PAGE_SIZE)
#define PMM_BITMAP_SIZE ((PMM_BITMAP_BITS + 7u) / 8u)

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

static uint8_t bitmap[PMM_BITMAP_SIZE] __attribute__((aligned(16)));

static uint32_t total_frames = 0;
static uint32_t used_frames = 0;
static uint32_t top_addr = 0;

static int frame_test(uint32_t f) {
	return (bitmap[f / 8] >> (f % 8)) & 1u;
}

static void frame_set(uint32_t f) {
	bitmap[f / 8] |= (uint8_t)(1u << (f % 8));
}

static void frame_clear(uint32_t f) {
	bitmap[f / 8] &= (uint8_t)~(1u << (f % 8));
}

static void range_free(uint32_t start, uint32_t len) {
	uint32_t end = start + len;
	if (end > PMM_MAX_PHYS) {
		end = PMM_MAX_PHYS;
	}
	if (start >= end) {
		return;
	}
	uint32_t fstart = start >> PAGE_SHIFT;
	uint32_t fend = (end + PAGE_SIZE - 1u) >> PAGE_SHIFT;
	for (uint32_t f = fstart; f < fend; f++) {
		if (frame_test(f)) {
			frame_clear(f);
			total_frames++;
		}
	}
	if (end > top_addr) {
		top_addr = end;
	}
}

static void range_reserve(uint32_t start, uint32_t len) {
	uint32_t end = start + len;
	if (end > PMM_MAX_PHYS) {
		end = PMM_MAX_PHYS;
	}
	if (start >= end) {
		return;
	}
	uint32_t fstart = start >> PAGE_SHIFT;
	uint32_t fend = (end + PAGE_SIZE - 1u) >> PAGE_SHIFT;
	for (uint32_t f = fstart; f < fend; f++) {
		if (!frame_test(f)) {
			frame_set(f);
			total_frames--;
		}
	}
}

void pmm_init(const struct multiboot_info *mbi) {
	for (uint32_t i = 0; i < sizeof(bitmap); i++) {
		bitmap[i] = 0xFF;
	}

	if (mbi != NULL && (mbi->flags & 1u) != 0u && mbi->mmap_length != 0u) {
		for (uint32_t addr = mbi->mmap_addr;
			 addr < mbi->mmap_addr + mbi->mmap_length;) {
			const struct multiboot_mmap_entry *e =
				(const struct multiboot_mmap_entry *)addr;
			if (e->type == 1u && e->addr_high == 0u && e->len_high == 0u) {
				range_free(e->addr_low, e->len_low);
			}
			addr += e->size + 4u;
		}
	} else if (mbi != NULL && (mbi->flags & 1u) != 0u) {
		range_free(0x100000u, (uint32_t)mbi->mem_upper * 1024u);
	} else {
		range_free(0x100000u, 32u * 1024u * 1024u);
	}

	range_reserve(0u, 0x100000u);
	range_reserve((uint32_t)(uintptr_t)&_kernel_start,
				  (uint32_t)(uintptr_t)&_kernel_end -
					  (uint32_t)(uintptr_t)&_kernel_start);
	if (mbi != NULL) {
		range_reserve((uint32_t)(uintptr_t)mbi, 0x1000u);
		range_reserve(mbi->mmap_addr, mbi->mmap_length);
	}
}

uint32_t pmm_alloc_frame(void) {
	for (uint32_t f = 0; f < PMM_BITMAP_BITS; f++) {
		if (!frame_test(f)) {
			frame_set(f);
			used_frames++;
			return f << PAGE_SHIFT;
		}
	}
	return 0u;
}

void pmm_free_frame(uint32_t addr) {
	uint32_t f = addr >> PAGE_SHIFT;
	if (f >= PMM_BITMAP_BITS || frame_test(f)) {
		return;
	}
	frame_set(f);
	used_frames--;
}

uint32_t pmm_total_frames(void) {
	return total_frames;
}

uint32_t pmm_free_frames(void) {
	return total_frames - used_frames;
}

uint32_t pmm_top_addr(void) {
	return top_addr;
}