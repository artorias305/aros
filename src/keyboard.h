#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);
void keyboard_irq_handler(void);
int keyboard_read_char(void);

#endif
