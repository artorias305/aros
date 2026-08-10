#include <stdarg.h>
#include <stdint.h>

#include "kprintf.h"
#include "serial.h"
#include "vga.h"

static void kputc(char c) {
	if (c == '\n') {
		serial_write_byte('\r');
	}
	serial_write_byte((uint8_t)c);
	vga_write_char(c);
}

static void kputs(const char *s) {
	while (*s != '\0') {
		kputc(*s++);
	}
}

static void put_unsigned(uint32_t value, unsigned base, int upper) {
	static const char lower_digits[] = "0123456789abcdef";
	static const char upper_digits[] = "0123456789ABCDEF";
	const char *digits = upper ? upper_digits : lower_digits;
	char buf[32];
	int i = 0;
	do {
		buf[i++] = digits[value % base];
		value /= base;
	} while (value != 0);
	while (i > 0) {
		kputc(buf[--i]);
	}
}

static void put_int(int value) {
	if (value < 0) {
		kputc('-');
		put_unsigned((uint32_t)(-(value + 1)) + 1u, 10, 0);
	} else {
		put_unsigned((uint32_t)value, 10, 0);
	}
}

void kprintf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	while (*fmt != '\0') {
		if (*fmt != '%') {
			kputc(*fmt++);
			continue;
		}

		fmt++;
		switch (*fmt) {
		case 's':
			kputs(va_arg(ap, const char *));
			break;
		case 'c':
			kputc((char)va_arg(ap, int));
			break;
		case 'd':
			put_int(va_arg(ap, int));
			break;
		case 'u':
			put_unsigned(va_arg(ap, uint32_t), 10, 0);
			break;
		case 'x':
			put_unsigned(va_arg(ap, uint32_t), 16, 0);
			break;
		case 'X':
			put_unsigned(va_arg(ap, uint32_t), 16, 1);
			break;
		case 'p':
			kputc('0');
			kputc('x');
			put_unsigned(va_arg(ap, uint32_t), 16, 0);
			break;
		case '%':
			kputc('%');
			break;
		default:
			kputc('%');
			kputc(*fmt);
			break;
		}

		if (*fmt != '\0') {
			fmt++;
		}
	}

	va_end(ap);
}
