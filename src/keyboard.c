#include "keyboard.h"

#include "io.h"
#include "serial.h"
#include "vga.h"

#define PS2_DATA 0x60u
#define PS2_STATUS 0x64u
#define PS2_CMD 0x64u

static const char keymap[128] = {
	0,	 27,  '1',	'2',  '3',	'4', '5', '6',	'7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q',	'w', 'e', 'r',	't', 'y', 'u', 'i',
	'o', 'p', '[',	']',  '\n', 0,	 'a', 's',	'd', 'f', 'g', 'h',
	'j', 'k', 'l',	';',  '\'', '`', 0,	  '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm',	',',  '.',	'/', 0,	  '*',	0,	 ' ',
};

static const char shiftmap[128] = {
	0,	 27,  '!',	'@',  '#',	'$', '%', '^', '&', '*', '(', ')',
	'_', '+', '\b', '\t', 'Q',	'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{',	'}',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	':',  '"',	'~', 0,	  '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	'<',  '>',	'?', 0,	  '*', 0,	' ',
};

static int shift = 0;

void keyboard_init(void) {
	while ((inb(PS2_STATUS) & 1u) != 0u) {
		inb(PS2_DATA);
	}
	while ((inb(PS2_STATUS) & 2u) != 0u) {
	}
	outb(PS2_CMD, 0xAE);
}

void keyboard_irq_handler(void) {
	uint8_t sc = inb(PS2_DATA);

	if (sc == 0x2A || sc == 0x36) {
		shift = 1;
		return;
	}
	if (sc == 0xAA || sc == 0xB6) {
		shift = 0;
		return;
	}
	if ((sc & 0x80u) != 0u || sc >= 128) {
		return;
	}

	char c = shift ? shiftmap[sc] : keymap[sc];
	if (c == 0) {
		return;
	}

	if (c == '\n') {
		serial_write_byte('\r');
	}
	vga_write_char(c);
	serial_write_byte(c);
}
