CC      := clang
LD      := ld.lld
TARGET  := i386-none-elf
CFLAGS  := --target=$(TARGET) -std=c11 -ffreestanding -fno-builtin \
           -fno-stack-protector -fno-pic -mno-sse -mno-mmx -msoft-float \
           -O2 -g -Wall -Wextra
LDFLAGS := -m elf_i386 -T src/linker.ld -nostdlib

OBJS := build/main.o build/serial.o build/vga.o build/gdt.o build/idt.o build/keyboard.o build/kprintf.o

all: build/kernel.elf

build/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

run: build/kernel.elf
	qemu-system-i386 -kernel build/kernel.elf -serial stdio

clean:
	rm -rf build

.PHONY: all run clean
