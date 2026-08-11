
#include "shell.h"

#include <stddef.h>
#include <stdint.h>

#include "io.h"
#include "keyboard.h"
#include "kprintf.h"
#include "pmm.h"
#include "serial.h"
#include "timer.h"
#include "vga.h"

#define LINE_MAX 128

static char line[LINE_MAX];
static size_t len = 0;

static void cmd_help(const char *arg) {
	(void)arg;
	kprintf("commands:\n");
	kprintf("  help        show this list\n");
	kprintf("  about       kernel version\n");
	kprintf("  echo <txt>  print text\n");
	kprintf("  clear       clear screen\n");
	kprintf("  reboot      reset the machine\n");
	kprintf("  uptime      kernel uptime\n");
	kprintf("  sleep <s>   pause for s seconds\n");
	kprintf("  meminfo     memory usage\n");
}

static void cmd_about(const char *arg) {
	(void)arg;
	kprintf("aros 0.0.1 -- i386, clang, no userspace\n");
}

static void cmd_clear(const char *arg) {
	(void)arg;
	vga_clear();
}

static void cmd_echo(const char *arg) {
	kprintf("%s\n", arg);
}

static void cmd_reboot(const char *arg) {
	(void)arg;
	kprintf("rebooting...\n");
	outb(0x64, 0xFE);
	for (;;) {
		__asm__ volatile("hlt");
	}
}

static void cmd_uptime(const char *arg) {
	(void)arg;
	uint32_t t = timer_ticks();
	uint32_t ms = (t % 100) * 10;
	kprintf("uptime: %u.%u%u s (%u ticks)\n", t / 100, ms / 10, ms % 10, t);
}

static void cmd_sleep(const char *arg) {
	int secs = 0;
	for (const char *p = arg; *p >= '0' && *p <= '9'; p++) {
		secs = secs * 10 + (*p - '0');
	}
	if (secs <= 0 || secs > 60) {
		kprintf("usage: sleep <secs>\n");
		return;
	}
	kprintf("sleeping for %d s...\n", secs);
	sleep_ticks((uint32_t)secs * 100);
}

static void cmd_meminfo(const char *arg) {
	(void)arg;
	kprintf("phys: %u KB usable, %u KB free (4 KB pages)\n",
			pmm_total_frames() * 4u, pmm_free_frames() * 4u);
}

struct command {
	const char *name;
	void (*func)(const char *arg);
};

static const struct command commands[] = {
	{"help", cmd_help},	  {"about", cmd_about},	  {"clear", cmd_clear},
	{"echo", cmd_echo},	  {"reboot", cmd_reboot}, {"echo", cmd_echo},
	{"sleep", cmd_sleep}, {"uptime", cmd_uptime}, {"meminfo", cmd_meminfo}};

static int str_eq(const char *a, const char *b) {
	while (*a != '\0' && *b != '\0') {
		if (*a++ != *b++) {
			return 0;
		}
	}
	return *a == *b;
}

static void shell_exec(char *cmdline) {
	while (*cmdline == ' ' || *cmdline == '\t') {
		cmdline++;
	}
	if (*cmdline == '\0') {
		return;
	}

	char *word_end = cmdline;
	while (*word_end != '\0' && *word_end != ' ' && *word_end != '\t') {
		word_end++;
	}

	char saved = *word_end;
	*word_end = '\0';

	char *arg = word_end;
	if (saved != '\0') {
		arg++;
		while (*arg == ' ' || *arg == '\t') {
			arg++;
		}
	}

	size_t ncmd = sizeof(commands) / sizeof(commands[0]);
	for (size_t i = 0; i < ncmd; i++) {
		if (str_eq(cmdline, commands[i].name)) {
			commands[i].func(arg);
			return;
		}
	}

	kprintf("unknown command '%s' -- try 'help'\n", cmdline);
}

void shell_run(void) {
	kprintf("aros 0.0.1\n");
	kprintf("type 'help' for commands\n");

	kprintf("aros> ");
	for (;;) {
		int c = keyboard_read_char();
		if (c < 0) {
			__asm__ volatile("hlt");
			continue;
		}

		if (c == '\n') {
			kprintf("\n");
			line[len] = '\0';
			shell_exec(line);
			len = 0;
			kprintf("aros> ");
		} else if (c == '\b') {
			if (len > 0) {
				len--;
				serial_write_byte('\b');
				serial_write_byte(' ');
				serial_write_byte('\b');
				vga_write_char('\b');
			}
		} else if (len < LINE_MAX - 1) {
			line[len++] = (char)c;
			serial_write_byte((uint8_t)c);
			vga_write_char((char)c);
		}
	}
}
