#include "acpi.h"

#include <stdint.h>
#include <stddef.h>

#include "io.h"
#include "paging.h"

#define ACPI_WIN 0x60000000u
#define PAGE_SIZE 4096u
#define PAGE_MASK (PAGE_SIZE - 1u)

#define RSDP_SIG "RSD PTR "
#define FACP_SIG "FACP"

#define SLP_EN 0x2000u
#define SLP_TYP_MASK 0x1C00u

struct rsdp {
	uint8_t signature[8];
	uint8_t checksum;
	uint8_t oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xsdt_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed));

static uint16_t pm1a_cnt_blk;

static void mem_copy(void *dst, const void *src, size_t n) {
	uint8_t *d = dst;
	const uint8_t *s = src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
}

static int mem_equal(const void *a, const void *b, size_t n) {
	const uint8_t *x = a;
	const uint8_t *y = b;
	for (size_t i = 0; i < n; i++) {
		if (x[i] != y[i]) {
			return 0;
		}
	}
	return 1;
}

static void *acpi_map(uint32_t phys) {
	paging_map_page(ACPI_WIN, phys & ~PAGE_MASK);
	return (void *)(ACPI_WIN + (phys & PAGE_MASK));
}

static void acpi_copy(uint32_t phys, void *dst, size_t n) {
	uint8_t *d = dst;
	while (n > 0) {
		size_t chunk = PAGE_SIZE - (phys & PAGE_MASK);
		if (chunk > n) {
			chunk = n;
		}
		mem_copy(d, acpi_map(phys), chunk);
		phys += (uint32_t)chunk;
		d += chunk;
		n -= chunk;
	}
}

static uint8_t checksum8(const uint8_t *p, size_t n) {
	uint8_t sum = 0;
	for (size_t i = 0; i < n; i++) {
		sum += p[i];
	}
	return sum;
}

static uint8_t table_checksum(uint32_t addr, uint32_t len) {
	uint8_t sum = 0;
	while (len > 0) {
		uint32_t n = PAGE_SIZE - (addr & PAGE_MASK);
		if (n > len) {
			n = len;
		}
		sum += checksum8(acpi_map(addr), n);
		addr += n;
		len -= n;
	}
	return sum;
}

static int rsdp_match(uint32_t a, struct rsdp *out) {
	uint8_t buf[36];
	acpi_copy(a, buf, sizeof(buf));
	if (!mem_equal(buf, RSDP_SIG, 8) || checksum8(buf, 20u) != 0) {
		return 0;
	}
	mem_copy(out, buf, sizeof(*out));
	return 1;
}

static int find_rsdp(struct rsdp *out) {
	uint16_t ebda_seg = 0;
	acpi_copy(0x40E, &ebda_seg, sizeof(ebda_seg));
	uint32_t ebda = (uint32_t)ebda_seg << 4;
	if (ebda >= 0x400u && ebda < 0xA0000u) {
		for (uint32_t a = ebda; a < ebda + 0x400u; a += 16u) {
			if (rsdp_match(a, out)) {
				return 1;
			}
		}
	}
	for (uint32_t a = 0xE0000u; a < 0x100000u; a += 16u) {
		if (rsdp_match(a, out)) {
			return 1;
		}
	}
	return 0;
}

static int fadt_pm1cnt(uint32_t addr) {
	char sig[4];
	uint32_t blk;
	uint8_t len;
	acpi_copy(addr, sig, sizeof(sig));
	if (!mem_equal(sig, FACP_SIG, 4)) {
		return 0;
	}
	acpi_copy(addr + 64u, &blk, sizeof(blk));
	acpi_copy(addr + 89u, &len, sizeof(len));
	if (blk == 0 || len < 2u) {
		return 0;
	}
	pm1a_cnt_blk = (uint16_t)blk;
	return 1;
}

static int walk_tables(uint32_t root, uint32_t len) {
	char sig[4];
	acpi_copy(root, sig, sizeof(sig));
	uint32_t step = (sig[0] == 'X') ? 8u : 4u;
	uint32_t n = (len - 36u) / step;
	for (uint32_t i = 0; i < n; i++) {
		uint32_t ent = 0;
		acpi_copy(root + 36u + i * step, &ent, sizeof(ent));
		if (ent != 0 && fadt_pm1cnt(ent)) {
			return 1;
		}
	}
	return 0;
}

int acpi_init(void) {
	struct rsdp rsdp;
	if (!find_rsdp(&rsdp)) {
		return 0;
	}
	uint32_t root = (rsdp.revision >= 2u) ? (uint32_t)rsdp.xsdt_address
										  : rsdp.rsdt_address;
	if (root == 0) {
		return 0;
	}
	uint32_t len = 0;
	acpi_copy(root + 4u, &len, sizeof(len));
	if (len < 36u || table_checksum(root, len) != 0) {
		return 0;
	}
	return walk_tables(root, len);
}

int acpi_poweroff(void) {
	if (pm1a_cnt_blk == 0) {
		return -1;
	}
	uint16_t v = inw(pm1a_cnt_blk);
	v &= ~SLP_TYP_MASK;
	v &= ~SLP_EN;
	outw(pm1a_cnt_blk, v | SLP_EN);
	for (;;) {
		__asm__ volatile("hlt");
	}
}
