#include <stdint.h>

#include "serial.h"

#define MB_ALIGN (1u << 0)
#define MB_MEMINFO (1u << 1)
#define MB_MAGIC 0x1BADB002u
#define MB_FLAGS (MB_ALIGN | MB_MEMINFO)

__attribute__((section(".multiboot"), used, aligned(4)))
static const struct {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
} multiboot_header = {
    .magic = MB_MAGIC,
    .flags = MB_FLAGS,
    .checksum = -(MB_MAGIC + MB_FLAGS),
};

static uint8_t stack[16384] __attribute__((aligned(16), section(".bss")));

__attribute__((naked, noreturn))
void _start(void) {
    __asm__ volatile (
        "movl %0, %%esp\n\t"
        "call kernel_main\n\t"
        :
        : "r"(stack + sizeof(stack))
        : "memory");
}

void kernel_main(void) {
    volatile uint16_t *vga_buffer = (volatile uint16_t *)0xB8000;
    const uint16_t color_attribute = 0x0F00;
    const char *msg = "Hello from C i386 Kernel!";

    serial_init();

    for (size_t i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = color_attribute | ' ';
    }

    serial_print("Hello from serial!\n");

    for (size_t i = 0; msg[i] != '\0'; i++) {
        vga_buffer[i] = color_attribute | msg[i];
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
