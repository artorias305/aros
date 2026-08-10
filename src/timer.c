#include <stdint.h>

#include "io.h"
#include "timer.h"

#define PIT_CH0 0x40u
#define PIT_CMD 0x43u
#define PIT_FREQ 1193182u

static volatile uint32_t ticks = 0;

void timer_irq_handler(void) { ticks++; }

void timer_init(uint32_t hz) {
	if (hz == 0) {
		hz = 100;
	}

	uint32_t divisor = (PIT_FREQ + hz / 2) / hz;
	if (divisor > 65535) {
		divisor = 65535;
	}
	if (divisor == 0) {
		divisor = 1;
	}

	outb(PIT_CMD, 0x36); // channel 0, lobyte/hibyte, mode 3, binary
	outb(PIT_CH0, divisor & 0xFF);
	outb(PIT_CH0, (divisor >> 8) & 0xFF);
}

uint32_t timer_ticks() { return ticks; }

void sleep_ticks(uint32_t n) {
	uint32_t start = ticks;
	while (ticks - start < n) {
		__asm__ volatile("hlt");
	}
}
