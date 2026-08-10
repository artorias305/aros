#include "vga.h"

#include <stddef.h>
#include <stdint.h>

#include "io.h"

static uint8_t *VGA_BUFFER = (uint8_t *)0xB8000;
static const size_t VGA_COLS = 80;
static const size_t VGA_ROWS = 25;

static size_t cursor_row = 0;
static size_t cursor_col = 0;

static const uint8_t VGA_COLOR = 0x07;

static void vga_scroll(void) {
	size_t row, col;
	for (row = 1; row < VGA_ROWS; row++) {
		size_t dst = (row - 1) * VGA_COLS * 2;
		size_t src = row * VGA_COLS * 2;
		for (col = 0; col < VGA_COLS * 2; col++) {
			VGA_BUFFER[dst + col] = VGA_BUFFER[src + col];
		}
	}
	for (col = 0; col < VGA_COLS; col++) {
		size_t index = ((VGA_ROWS - 1) * VGA_COLS + col) * 2;
		VGA_BUFFER[index] = ' ';
		VGA_BUFFER[index + 1] = VGA_COLOR;
	}
	cursor_row = VGA_ROWS - 1;
	cursor_col = 0;
}

static void vga_update_cursor(void) {
	uint16_t pos = (uint16_t)(cursor_row * VGA_COLS + cursor_col);
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_init(void) {
	cursor_row = 0;
	cursor_col = 0;
	vga_clear();
	vga_update_cursor();
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
	} else if (c == '\b') {
		if (cursor_col > 0) {
			--cursor_col;
		} else if (cursor_row > 0) {
			--cursor_row;
			cursor_col = VGA_COLS - 1;
		}
		size_t index = (cursor_row * VGA_COLS + cursor_col) * 2;
		VGA_BUFFER[index] = ' ';
		VGA_BUFFER[index + 1] = VGA_COLOR;
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
		vga_scroll();
	}

	vga_update_cursor();
}

void vga_write_str(const char *s) {
	if (s == NULL)
		return;
	while (*s != '\0')
		vga_write_char(*s++);
}
