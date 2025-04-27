// SPDX-License-Identifier: GPL-2.0
/*
 * (C) Copyright 2019 Rockchip Electronics Co., Ltd.
 */

#include <stdio.h>
#include "types.h"
#include "cache.h"
#include <sys/syscall.h>

void dcacheflush(ul start, ul end)
{
#if defined (__aarch64__)
	ul cache_line;

	__asm("mrs %0, ctr_el0":"+r" (cache_line):);
	cache_line = 4 << ((cache_line >> 16) & 0xf);
	printf("cache line:%u\n", cache_line);
	start &= ~(cache_line - 1);

	for (; start < end; start += cache_line) {
		__asm("dc civac, %0":"+r" (start):);
	}
#else
    printf("flush cache\n");
    syscall(__ARM_NR_cacheflush, start, end, 0);
#endif
}
