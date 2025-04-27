// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "va_2_pa.h"

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((unsigned long long)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

unsigned long read_pagemap(unsigned long virt_addr)
{
    FILE * f;
    unsigned char c_buf[PAGEMAP_ENTRY];
    unsigned int status, i, c;
    ull page_size, file_offset;
    unsigned long phy_addr = 0, page;

	f = fopen("/proc/self/pagemap", "rb");
	if (!f) {
		printf("Error! Cannot open /proc/self/pagemap\n");
		return -1;
	}
    page_size = getpagesize();

	file_offset = virt_addr / page_size * PAGEMAP_ENTRY;
	//printf("Vaddr: 0x%lx, Page_size: %lld, Entry_size: %d\n", virt_addr, page_size, PAGEMAP_ENTRY);
	///printf("Reading %s at 0x%llx\n", "/proc/self/pagemap", (unsigned long long)file_offset);
	status = fseek(f, file_offset, SEEK_SET);
	if (status) {
		perror("Failed to do fseek for get physical address!\n");
		return -1;
	}
	errno = 0;
	page = 0;
	for(i=0; i < PAGEMAP_ENTRY; i++){
		c = getc(f);
		if (c==EOF) {
			printf("\nReached end of the file\n");
			return -1;
		}
		if (is_bigendian())
			c_buf[i] = c;
		else
			c_buf[PAGEMAP_ENTRY - i - 1] = c;
		//printf("[%d]0x%x ", i, c);
	}
	for(i=0; i < PAGEMAP_ENTRY; i++){
		//printf("%d ",c_buf[i]);
		page = (page << 8) + c_buf[i];
	}
	//printf("Result: 0x%llx\n", (unsigned long long) page);
	if(GET_BIT(page, 63)) {
		uint64_t pfn = GET_PFN(page);
        phy_addr = pfn * page_size + virt_addr % page_size;
		//printf("PFN: 0x%llx (0x%llx)\n", pfn, phy_addr);
	} else {
		printf("Page not present\n");
    }
	if(GET_BIT(page, 62))
		printf("Page swapped\n");
	fclose(f);
	return phy_addr;
}
