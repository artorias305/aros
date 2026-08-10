#include <stdint.h>

#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "serial.h"
#include "vga.h"

#define MB_ALIGN (1u << 0)
#define MB_MEMINFO (1u << 1)
#define MB_MAGIC 0x1BADB002u
#define MB_FLAGS (MB_ALIGN | MB_MEMINFO)

__attribute__((section(".multiboot"), used, aligned(4))) static const struct {
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
} multiboot_header = {
	.magic = MB_MAGIC,
	.flags = MB_FLAGS,
	.checksum = -(MB_MAGIC + MB_FLAGS),
};

static uint8_t stack[16384] __attribute__((aligned(16), section(".bss")));

__attribute__((naked, noreturn)) void _start(void) {
	__asm__ volatile("movl %0, %%esp\n\t"
					 "call kernel_main\n\t"
					 :
					 : "r"(stack + sizeof(stack))
					 : "memory");
}

void kernel_main(void) {
	serial_init();
	serial_print("[BOOT] serial initialized\n");

	vga_init();

	gdt_init();
	serial_print("[OK]  GDT loaded\n");

	idt_init();
	serial_print("[OK]  IDT loaded\n");

	keyboard_init();
	serial_print("[OK]  keyboard initialized\n");

	__asm__ volatile("sti");

	for (;;) {
		__asm__ volatile("hlt");
	}
}
