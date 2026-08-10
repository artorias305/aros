#include "vga.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t *VGA_BUFFER = (uint8_t *)0xB8000;
static const size_t VGA_COLS = 80;
static const size_t VGA_ROWS = 25;

static size_t cursor_row = 0;
static size_t cursor_col = 0;

static const uint8_t VGA_COLOR = 0x07;

void vga_init(void) {
	cursor_row = 0;
	cursor_col = 0;
	vga_clear();
}

void vga_clear(void) {
	for (size_t i = 0; i < VGA_COLS * VGA_ROWS; ++i) {
		VGA_BUFFER[i * 2] = ' ';
		VGA_BUFFER[i * 2 + 1] = VGA_COLOR;
	}
}

void vga_write_char(char c) {
	if (c == '\n') {
		++cursor_row;
		cursor_col = 0;
	} else {
		size_t index = (cursor_row * VGA_COLS + cursor_col) * 2;

		VGA_BUFFER[index] = (uint8_t)c;
		VGA_BUFFER[index + 1] = VGA_COLOR;

		++cursor_col;

		if (cursor_col >= VGA_COLS) {
			cursor_col = 0;
			++cursor_row;
		}
	}

	if (cursor_row >= VGA_ROWS) {
		cursor_row = 0;
		cursor_col = 0;
	}
}

void vga_write_str(const char *s) {
	if (s == NULL)
		return;
	while (*s != '\0')
		vga_write_char(*s++);
}