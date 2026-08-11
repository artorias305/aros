#include "paging.h"

#include <stdint.h>

#include "pmm.h"
#include "serial.h"

#define PAGE_SIZE 4096u
#define PAGE_TABLE_ENTRIES 1024u
#define PAGE_DIR_ENTRIES 1024u

#define PTE_PRESENT 0x1u
#define PTE_WRITABLE 0x2u

static uint32_t page_directory[PAGE_DIR_ENTRIES] __attribute__((aligned(4096)));

void paging_init(void) {
	uint32_t top = pmm_top_addr();
	uint32_t npt = (top + (PAGE_DIR_ENTRIES * PAGE_SIZE) - 1u) /
				   (PAGE_DIR_ENTRIES * PAGE_SIZE);
	if (npt == 0u) {
		npt = 1u;
	}
	if (npt > PAGE_DIR_ENTRIES) {
		npt = PAGE_DIR_ENTRIES;
	}

	for (uint32_t i = 0; i < npt; i++) {
		uint32_t pt_addr = pmm_alloc_frame();
		if (pt_addr == 0u) {
			serial_print("[PANIC] no frame for page table\n");
			for (;;) {
				__asm__ volatile("hlt");
			}
		}

		uint32_t *pt = (uint32_t *)pt_addr;
		for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
			pt[j] = 0u;
		}
		for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
			pt[j] = (i * PAGE_DIR_ENTRIES + j) * PAGE_SIZE | PTE_PRESENT |
					PTE_WRITABLE;
		}

		page_directory[i] = pt_addr | PTE_PRESENT | PTE_WRITABLE;
	}

	__asm__ volatile("movl %0, %%cr3\n\t"
					 "movl %%cr0, %%eax\n\t"
					 "orl $0x80000000, %%eax\n\t"
					 "movl %%eax, %%cr0\n\t"
					 "jmp 1f\n\t"
					 "1:\n\t"
					 :
					 : "r"(page_directory)
					 : "eax", "memory");
}

void paging_map_page(uint32_t virt, uint32_t phys) {
	uint32_t pd_idx = virt >> 22;
	uint32_t pt_idx = (virt >> 12) & (PAGE_TABLE_ENTRIES - 1u);

	uint32_t pt_addr;
	if ((page_directory[pd_idx] & PTE_PRESENT) == 0u) {
		pt_addr = pmm_alloc_frame();
		if (pt_addr == 0u) {
			serial_print("[PANIC] no frame for page table\n");
			for (;;) {
				__asm__ volatile("hlt");
			}
		}
		uint32_t *pt = (uint32_t *)pt_addr;
		for (uint32_t j = 0; j < PAGE_TABLE_ENTRIES; j++) {
			pt[j] = 0u;
		}
		page_directory[pd_idx] = pt_addr | PTE_PRESENT | PTE_WRITABLE;
	} else {
		pt_addr = page_directory[pd_idx] & 0xFFFFF000u;
	}

	uint32_t *pt = (uint32_t *)pt_addr;
	pt[pt_idx] = (phys & 0xFFFFF000u) | PTE_PRESENT | PTE_WRITABLE;

	__asm__ volatile("invlpg %0" : : "m"(*(uint32_t *)virt) : "memory");
}
