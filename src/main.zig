const std = @import("std");

// multiboot 1 constants
const ALIGN = 1 << 0;
const MEMINFO = 1 << 1;
const FLAGS = ALIGN | MEMINFO;
const MAGIC = 0x1BADB002;
const CHECKSUM = @as(i32, @bitCast(~@as(u32, MAGIC + FLAGS) +% 1));

// force the linker to keep the multiboot header in the final binary
export const multiboot align(4) linksection(".multiboot") = extern struct {
    magic: i32 = MAGIC,
    flags: i32 = FLAGS,
    checksum: i32 = CHECKSUM,
}{};

export var stack: [16384]u8 align(16) linksection(".bss") = undefined;

export fn _start() callconv(.naked) noreturn {
    asm volatile (
        \\movl %[stack_top], %%esp
        \\call kernel_main
        :
        : [stack_top] "i" (&stack[stack.len..].ptr),
    );
    while (true) {}
}

export fn kernel_main() noreturn {
    const vga_buffer = @as([*]volatile u16, @ptrFromInt(0xB8000));
    const msg = "Hello from Zig i386 Bare Bones!";
    const color_attribute: u16 = 0x0F00;

    for (0..80 * 25) |i| {
        vga_buffer[i] = color_attribute | ' ';
    }

    for (msg, 0..) |char, i| {
        vga_buffer[i] = color_attribute | char;
    }

    while (true) {
        asm volatile ("hlt");
    }
}
