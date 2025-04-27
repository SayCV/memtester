// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd.
 */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "types.h"
#include "io_map.h"

#define IO_TYPE_1_1_16			0
#define IO_TYPE_1_1_32			1
#define IO_TYPE_1_2			2
#define IO_TYPE_2			3

#define SYS_REG_DEC_BW(n, ch)		(2 >> (((n) >> (2 + 16 * (ch))) & 0x3))

static u32 io_type;

u32 io_bw;

struct chip_info {
	char chip_name[10];
	uint32_t osreg_addr;
};

struct chip_info chip_info[] = {
	{ .chip_name = "rv1106",
	  .osreg_addr = 0xff020208
	},
	{ .chip_name = "rv1108",
	  .osreg_addr = 0x10300588
	},
	{ .chip_name = "rv1109",
	  .osreg_addr = 0xfe020208
	},
	{ .chip_name = "rv1126",
	  .osreg_addr = 0xfe020208
	},
	{ .chip_name = "rk1808",
	  .osreg_addr = 0xfe020208
	},
	{ .chip_name = "rk3036",
	  .osreg_addr = 0x200081cc
	},
	{ .chip_name = "rk3126b",
	  .osreg_addr = 0x200081cc
	},
	{ .chip_name = "rk3126c",
	  .osreg_addr = 0x200081cc
	},
	{ .chip_name = "rk3126",
	  .osreg_addr = 0x200081cc
	},
	{ .chip_name = "rk3128",
	  .osreg_addr = 0x200081cc
	},
	{ .chip_name = "rk3188",
	  .osreg_addr = 0x20004048
	},
	{ .chip_name = "rk3228h",
	  .osreg_addr = 0xff1005d0
	},
	{ .chip_name = "rk3228",
	  .osreg_addr = 0x110005d0
	},
	{ .chip_name = "rk3229",
	  .osreg_addr = 0x110005d0
	},
	{ .chip_name = "rk3288",
	  .osreg_addr = 0xff73009c
	},
	{ .chip_name = "rk3308",
	  .osreg_addr = 0xff000208
	},
	{ .chip_name = "rk3326",
	  .osreg_addr = 0xff010208
	},
	{ .chip_name = "px30",
	  .osreg_addr = 0xff010208
	},
	{ .chip_name = "rk3328",
	  .osreg_addr = 0xff1005d0
	},
	{ .chip_name = "rk3368",
	  .osreg_addr = 0xff738208
	},
	{ .chip_name = "rk3399",
	  .osreg_addr = 0xff320308
	},
	{ .chip_name = "rk3528",
	  .osreg_addr = 0xff370248
	},
	{ .chip_name = "rk3562",
	  .osreg_addr = 0xff010208
	},
	{ .chip_name = "rk3566",
	  .osreg_addr = 0xfdc20208
	},
	{ .chip_name = "rk3568",
	  .osreg_addr = 0xfdc20208
	},
	{ .chip_name = "rk3588",
	  .osreg_addr = 0xfd58a208
	}
};

char* try_get_chip_name(void)
{
	FILE * file;
	char *chip_name = NULL;
	char buffer[100], buffer_new[100];
	int i, j, bytes;

	file = fopen("/proc/device-tree/compatible", "r");
	if (file == NULL) {
		printf("Failed to open /proc/device-tree/compatible\n");
		goto out;
	}

	bytes = fread(buffer, sizeof(char), sizeof(buffer), file);
	if (ferror(file)) {
		printf("Failed to read /proc/device-tree/compatible.\n");
		fclose(file);
		goto out;
	}
	fclose(file);

	for (i = 0, j = 0; i < bytes; i++) {
		if ((buffer[i] == '\0') && (i < (bytes - 1)))
			continue;
		else
			buffer_new[j++] = buffer[i];
	}

	for (i = 0; i < (sizeof(chip_info) / sizeof(chip_info[0])); i++) {
		if (strstr(buffer_new, chip_info[i].chip_name) != NULL) {
			chip_name = chip_info[i].chip_name;
			break;
		}
	}

out:
	printf("get chip name: %s\n", chip_name);
	return chip_name;
}

char* try_get_ddr_bw(char *chip_name)
{
	ull page_size;
	char *ddr_bw = NULL;
	int fd;
	uint32_t reg_addr = 0, reg_val = 0;
	uint32_t i, bw;

	if (chip_name == NULL)
		goto out;

	for (i = 0; i < (sizeof(chip_info) / sizeof(chip_info[0])); i++) {
		if (!strcasecmp(chip_name, chip_info[i].chip_name)) {
			reg_addr = chip_info[i].osreg_addr;
			break;
		}
	}

	if (reg_addr == 0)
		goto out;

	page_size = getpagesize();

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		printf("Failed to open /dev/mem\n");
		goto out;
	}

	void *map_base = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
			      reg_addr & ~(page_size - 1));
	if (map_base == (void*)-1) {
		printf("Failed to mmap /dev/mem\n");
		close(fd);
		goto out;
	}

	reg_val = (uint32_t)*(uint32_t *)(map_base + reg_addr % page_size);
	if (munmap(map_base, page_size) == -1) {
		printf("Failed to munmap /dev/mem");
		close(fd);
		goto out;
	}
	close(fd);

	bw = 8 << SYS_REG_DEC_BW(reg_val, 0);
	if (bw == 8)
		ddr_bw = "bw_x8";
	else if (bw == 16)
		ddr_bw = "bw_x16";
	else if (bw == 32)
		ddr_bw = "bw_x32";

out:
	printf("get ddr bw: %s\n", ddr_bw);
	return ddr_bw;
}

/* len should be 16byte align */
int data_cpu_2_io(void *p, u32 len)
{
	uchar *val = p;
	uchar buf[16];
	u32 i, j;

	if ((len % sizeof(buf)) || !len)
		return -1;

	if (io_type == IO_TYPE_1_2) {
		len /= sizeof(buf);
		for (j = 0; j < len; j++) {
			memset(buf, 0, sizeof(buf));
			for (i = 0; i < sizeof(buf); i++)
				buf[i] = val[(i % 4) * 4 + i / 4 + j * sizeof(buf)];
			memcpy(&val[j * sizeof(buf)], buf, sizeof(buf));
		}
	} else if (io_type == IO_TYPE_1_1_32) {
		len /= 8;
		for (j = 0; j < len; j++) {
			memset(buf, 0, sizeof(buf));
			for (i = 0; i < 8; i++)
				buf[i] = val[(i % 4) * 2 + i / 4 + j * 8];
			memcpy(&val[j * 8], buf, 8);
		}
	}
	/* IO_TYPE_2 and IO_TYPE_1_1_16 do nothing*/
	return 0;
}

void data_cpu_2_io_init(char *chip, char *ddr_bw)
{
	u32 osreg = 0;
	u32 bw;

	io_bw = IO_BW_32;
	io_type = IO_TYPE_2;

	if (chip == NULL) {
		chip = try_get_chip_name();
		if (chip == NULL)
			goto bw_type;
	}

	if (!strcasecmp(chip, "rk3036") ||
		!strcasecmp(chip, "rk3126") ||
		!strcasecmp(chip, "rk3126b") ||
		!strcasecmp(chip, "rk3126c")) {
		io_type = IO_TYPE_1_1_16;
	} else if (!strcasecmp(chip, "rk3228") ||
		!strcasecmp(chip, "rk3229") ||
		!strcasecmp(chip, "rk3368")) {
		io_type = IO_TYPE_1_2;
	} else if (!strcasecmp(chip, "rv1108") ||
		!strcasecmp(chip, "rk3308")) {
		io_type = IO_TYPE_1_2;
		io_bw = IO_BW_16;
	} else if (!strcasecmp(chip, "rk3128")) {
		osreg = 0x200081cc;
	} else if (!strcasecmp(chip, "rk3288")) {
		osreg = 0xff73009c;
	} else if (!strcasecmp(chip, "rk3188")) {
		osreg = 0x20004048;
	} else if (!strcasecmp(chip, "rk3328") ||
		!strcasecmp(chip, "rk3228h")||
		!strcasecmp(chip, "px30")||
		!strcasecmp(chip, "rk3326")||
		!strcasecmp(chip, "rk1808")||
		!strcasecmp(chip, "rv1126")||
		!strcasecmp(chip, "rv1109")) {
		io_type = IO_TYPE_2;
	} else {
		/* default IO_TYPE_2 */
		printf("no available chip info, using default maping\n");
		io_type = IO_TYPE_2;
	}

	if (osreg) {
		bw = (2 >> ((osreg >> 2) & 0x3));
		if (bw == 2)
			io_type = IO_TYPE_1_1_32;
		else
			io_type = IO_TYPE_1_1_16;
	}

bw_type:
	if (ddr_bw == NULL) {
		ddr_bw = try_get_ddr_bw(chip);
		if (ddr_bw == NULL)
			goto out;
	}

	if (!strcasecmp(ddr_bw, "bw_x32")) {
		io_bw = IO_BW_32;
	} else if (!strcasecmp(ddr_bw, "bw_x16")) {
		io_bw = IO_BW_16;
	} else if (!strcasecmp(ddr_bw, "bw_x8")) {
		io_bw = IO_BW_8;
	}

out:
	if (io_bw == IO_BW_32)
		printf("io bw x32\n");
	else if (io_bw == IO_BW_16)
		printf("io bw x16\n");
	else if (io_bw == IO_BW_8)
		printf("io bw x8\n");
}

