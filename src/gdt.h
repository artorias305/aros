#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define KERNEL_CS 0x08u
#define KERNEL_DS 0x10u
#define USER_CS 0x18u
#define USER_DS 0x20u
#define TSS_SEL 0x28u

void gdt_init(void);
void tss_set_esp0(uint32_t esp0);

#endif
