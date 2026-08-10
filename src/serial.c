#include "serial.h"

#define COM1 0x3F8u

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00); /* disable interrupts */
    outb(COM1 + 3, 0x80); /* enable DLAB */
    outb(COM1 + 0, 0x03); /* divisor low  -> 38400 baud */
    outb(COM1 + 1, 0x00); /* divisor high */
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, 1 stop bit */
    outb(COM1 + 2, 0xC7); /* FIFO on, clear, 14-byte threshold */
    outb(COM1 + 4, 0x0B); /* RTS/DSR set */
}

void serial_write_byte(uint8_t byte) {
    while ((inb(COM1 + 5) & 0x20u) == 0u) { /* wait for THR empty */
    }
    outb(COM1, byte);
}

void serial_write_all(const uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = bytes[i];
        if (byte == '\n') {
            serial_write_byte('\r');
        }
        serial_write_byte(byte);
    }
}

void serial_print(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    serial_write_all((const uint8_t *)str, len);
}
