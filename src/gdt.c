#include <stdint.h>

#include "gdt.h"

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t flags_limit_high;
	uint8_t base_high;
} __attribute__((packed));

struct gdtr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct tss {
	uint32_t back_link;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax, ecx, edx, ebx;
	uint32_t esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));

static void gdt_set_entry(struct gdt_entry *entry, uint32_t base,
			  uint32_t limit, uint8_t access, uint8_t flags)
{
	entry->limit_low = limit & 0xFFFF;
	entry->base_low = base & 0xFFFF;
	entry->base_mid = (base >> 16) & 0xFF;
	entry->access = access;
	entry->flags_limit_high = (flags << 4) | ((limit >> 16) & 0x0F);
	entry->base_high = (base >> 24) & 0xFF;
}

static struct gdt_entry gdt[6] __attribute__((aligned(8)));
static struct gdtr gdtr;
static struct tss tss;

void tss_set_esp0(uint32_t esp0)
{
	tss.esp0 = esp0;
}

void gdt_init(void)
{
	gdt_set_entry(&gdt[0], 0, 0, 0x00, 0x0); /* null */
	gdt_set_entry(&gdt[1], 0, 0xFFFFF, 0x9A, 0xC); /* kernel cs */
	gdt_set_entry(&gdt[2], 0, 0xFFFFF, 0x92, 0xC); /* kernel ds */
	gdt_set_entry(&gdt[3], 0, 0xFFFFF, 0xFA, 0xC); /* user cs */
	gdt_set_entry(&gdt[4], 0, 0xFFFFF, 0xF2, 0xC); /* user ds */

	tss.iomap_base = sizeof(struct tss);
	gdt_set_entry(&gdt[5], (uint32_t)(uintptr_t)&tss,
		      sizeof(struct tss) - 1, 0x89,
		      0x0); /* available 32-bit tss */

	gdtr.limit = sizeof(gdt) - 1;
	gdtr.base = (uint32_t)(uintptr_t)gdt;

	__asm__ volatile("lgdt %0\n\t"
			 "ljmp $0x08, $1f\n\t"
			 "1:\n\t"
			 "movw $0x10, %%ax\n\t"
			 "movw %%ax, %%ds\n\t"
			 "movw %%ax, %%es\n\t"
			 "movw %%ax, %%fs\n\t"
			 "movw %%ax, %%gs\n\t"
			 "movw %%ax, %%ss\n\t"
			 "movw %1, %%ax\n\t"
			 "ltr %%ax\n\t"
			 :
			 : "m"(gdtr), "i"(TSS_SEL)
			 : "ax", "memory");
}
