#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>

void serial_init(void);
void serial_write_byte(uint8_t byte);
void serial_write_all(const uint8_t *bytes, size_t len);
void serial_print(const char *str);

#endif
