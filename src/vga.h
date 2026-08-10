#ifndef VGA_H
#define VGA_H

// clears screen, resets cursor
void vga_init(void);
void vga_clear(void);
void vga_write_char(char c);
void vga_write_str(const char *s);

#endif