#include <stdint.h>

#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "serial.h"
#include "timer.h"
#include "gdt.h"

#define IDT_INT32 0x8Eu

#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA 0xA1u

struct idt_entry {
	uint16_t base_low;
	uint16_t sel;
	uint8_t zero;
	uint8_t flags;
	uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

struct regs {
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t vector;
	uint32_t eip, cs, eflags;
} __attribute__((packed));

static struct idt_entry idt[256] __attribute__((aligned(8)));
static struct idt_ptr idtp;

static void idt_set_gate(uint8_t n, void (*handler)(void)) {
	uint32_t base = (uint32_t)(uintptr_t)handler;
	idt[n].base_low = base & 0xFFFF;
	idt[n].base_high = (base >> 16) & 0xFFFF;
	idt[n].sel = KERNEL_CS;
	idt[n].zero = 0;
	idt[n].flags = IDT_INT32;
}

__attribute__((naked, used)) static void isr_common(void) {
	__asm__("pusha\n\t"
			"pushl %esp\n\t"
			"call isr_handler\n\t"
			"addl $4, %esp\n\t"
			"popa\n\t"
			"addl $8, %esp\n\t"
			"iret");
}

#define ISR_STUB(n)                                                            \
	__attribute__((naked)) static void isr_##n(void) {                         \
		__asm__("pushl $0\n\tpushl $" #n "\n\tjmp isr_common");                \
	}

#define ISR_STUB_ERR(n)                                                        \
	__attribute__((naked)) static void isr_##n(void) {                         \
		__asm__("pushl $" #n "\n\tjmp isr_common");                            \
	}

ISR_STUB(0)
ISR_STUB(1)
ISR_STUB(2)
ISR_STUB(3)
ISR_STUB(4)
ISR_STUB(5)
ISR_STUB(6)
ISR_STUB(7)
ISR_STUB_ERR(8)
ISR_STUB(9)
ISR_STUB_ERR(10)
ISR_STUB_ERR(11)
ISR_STUB_ERR(12)
ISR_STUB_ERR(13)
ISR_STUB_ERR(14)
ISR_STUB(15)
ISR_STUB(16)
ISR_STUB(17)
ISR_STUB(18)
ISR_STUB(19)
ISR_STUB(20)
ISR_STUB(21)
ISR_STUB(22)
ISR_STUB(23)
ISR_STUB(24)
ISR_STUB(25)
ISR_STUB(26)
ISR_STUB(27)
ISR_STUB(28) ISR_STUB(29) ISR_STUB(30) ISR_STUB(31) ISR_STUB(32) ISR_STUB(33)

	static void (*const isr_table[32])(void) = {
		isr_0,	isr_1,	isr_2,	isr_3,	isr_4,	isr_5,	isr_6,	isr_7,
		isr_8,	isr_9,	isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
		isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
		isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31,
};

static const char *const exception_names[32] = {
	"Divide by Zero",
	"Debug",
	"NMI",
	"Breakpoint",
	"Overflow",
	"Bound Range",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"Coprocessor Seg Overrun",
	"Invalid TSS",
	"Segment Not Present",
	"Stack Fault",
	"General Protection",
	"Page Fault",
	"Reserved",
	"x87 FP Error",
	"Alignment Check",
	"Machine Check",
	"SIMD FP Error",
	"Virtualization",
	"Control Protection",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Security",
	"Reserved",
};

static void pic_remap(void) {
	outb(PIC1_COMMAND, 0x11);
	outb(PIC2_COMMAND, 0x11);
	outb(PIC1_DATA, 0x20);
	outb(PIC2_DATA, 0x28);
	outb(PIC1_DATA, 0x04);
	outb(PIC2_DATA, 0x02);
	outb(PIC1_DATA, 0x01);
	outb(PIC2_DATA, 0x01);
	outb(PIC1_DATA, 0x00);
	outb(PIC2_DATA, 0x00);
}

void isr_handler(struct regs *r) {
	if (r->vector < 32) {
		serial_print("[EXC] ");
		serial_print(exception_names[r->vector]);
		serial_print(" (vector ");
		serial_write_hex(r->vector);
		serial_print(") at eip=");
		serial_write_hex(r->eip);
		if (r->vector == 14) {
			uint32_t cr2;
			__asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
			serial_print("  fault addr=");
			serial_write_hex(cr2);
		}
		serial_print("\n");
		for (;;) {
			__asm__ volatile("hlt");
		}
	}

	if (r->vector == 0x20) {
		timer_irq_handler();
	}

	if (r->vector == 0x21) {
		keyboard_irq_handler();
	}

	outb(PIC1_COMMAND, 0x20);
	if (r->vector >= 0x28) {
		outb(PIC2_COMMAND, 0x20);
	}
}

void idt_init(void) {
	for (int i = 0; i < 32; i++) {
		idt_set_gate((uint8_t)i, isr_table[i]);
	}

	idt_set_gate(0x20, isr_32); // IRQ0 timer
	idt_set_gate(0x21, isr_33); // IRQ1 keyboard

	idtp.limit = sizeof(idt) - 1;
	idtp.base = (uint32_t)(uintptr_t)idt;

	__asm__ volatile("lidt %0" : : "m"(idtp));

	pic_remap();
	outb(PIC1_DATA, 0xFC); // unmask IRQ0+IRQ1 only
	outb(PIC2_DATA, 0xFF); // mask all slave IRQs
}
