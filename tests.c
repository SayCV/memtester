/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2024 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtester.c.  See other comments in that file.
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "types.h"
#include "sizes.h"
#include "memtester.h"
#include "io_map.h"
#include "va_2_pa.h"
#include "cache.h"

char progress[] = "-\\|/";
#define PROGRESSLEN 4
#define PROGRESSOFTEN 2500
#define ONE 0x00000001UL

union {
    unsigned char bytes[UL_LEN/8];
    u32 val;
} mword8;

union {
    unsigned short u16s[UL_LEN/16];
    u32 val;
} mword16;

u32 fault_cnt = 0;

/* Function definitions. */

__attribute__((weak)) void dcacheflush(ul start, ul end)
{
	return;
}

int compare_regions(u32v *bufa, u32v *bufb, size_t count) {
    int r = 0;
    size_t i;
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    u32 read[2];
    u32 reread[2];
    off_t physaddr;

    for (i = 0; i < count; i++, p1++, p2++) {
        read[0] = *p1;
        read[1] = *p2;
        if (read[0] != read[1]) {
            dcacheflush((ul)p1, (ul)p1 + 4);
            dcacheflush((ul)p2, (ul)p2 + 4);
            reread[0] = *p1;
            reread[1] = *p2;
            if (use_phys) {
                physaddr = physaddrbase + (i * sizeof(u32v));
                fprintf(stdout,
                        "FAILURE: 0x%08x != 0x%08x (readA^readB:0x%08x) at physical address "
                        "0x%08lx, reread:0x%08x, 0x%08x (rereadA^readA:0x%08x, rereadB^readB:0x%08x).\n",
                        read[0], read[1], read[0] ^ read[1], physaddr,
			            reread[0], reread[1], reread[0] ^ read[0], reread[1] ^ read[1]);
            } else {
                fprintf(stdout,
                        "FAILURE: 0x%08x != 0x%08x (readA^readB:0x%08x) at offset 0x%08lx.\n"
                        "physical address: 0x%08lx, 0x%08lx, "
                        "reread:0x%08x, 0x%08x (rereadA^readA:0x%08x, rereadB^readB:0x%08x).\n",
                        read[0], read[1], read[0] ^ read[1], (ul) (i * sizeof(u32v)),
                        read_pagemap((ul)p1), read_pagemap((ul)p2),
                        reread[0], reread[1], reread[0] ^ read[0], reread[1] ^ read[1]);
            }
            /* printf("Skipping to next test..."); */
            r = -1;
        }
    }
    return r;
}

int test_stuck_address(u32v *bufa, size_t count) {
    u32v *p1 = bufa;
    u32 read, expect;
    u32 reread;
    unsigned int j;
    size_t i;
    off_t physaddr;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 16; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (u32v *) bufa;
        printf("setting %3u", j);
        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (u32v) p1 : ~((u32v) p1);
            *p1++;
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        p1 = (u32v *) bufa;
        for (i = 0; i < count; i++, p1++) {
            read = *p1;
            expect = ((j + i) % 2) == 0 ? (u32)(ul) p1 : ~((u32)(ul) p1);
            if (read != expect) {
                dcacheflush((ul)p1, (ul)p1 + 4);
                reread = *p1;
                if (use_phys) {
                    physaddr = physaddrbase + (i * sizeof(u32));
                    fprintf(stdout,
                            "FAILURE: possible bad address line at physical "
                            "address 0x%08lx:0x%08x != 0x%08x (0x%08x), reread:0x%08x(reread^read:0x%08x).\n",
                            physaddr, read, expect, read ^ expect, reread, reread ^ read);
                } else {
                    fprintf(stdout,
                             "FAILURE: possible bad address line at offset "
                            "0x%08lx.\n"
                            "physical address 0x%08lx:0x%08x != 0x%08x (0x%08x),"
                            "reread:0x%08x(reread^read:0x%08x).\n",
                            (ul) (i * sizeof(u32)), read_pagemap((ul)p1),
                            read, expect, read ^ expect, reread, reread ^ read);
                }
                printf("Skipping to next test...\n");
                fflush(stdout);
                fault_cnt++;
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_random_value(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    ul j = 0;
    size_t i;

    putchar(' ');
    fflush(stdout);
    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if (!(i % PROGRESSOFTEN)) {
            putchar('\b');
            putchar(progress[++j % PROGRESSLEN]);
            fflush(stdout);
        }
    }
    printf("\b \b");
    fflush(stdout);
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        if (!q) {
            q++;
        }
        *p1++ /= q;
        *p2++ /= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    size_t i;
    u32 q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 q;
    u32 data[4];
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        if (io_bw == IO_BW_32) {
            q = (j % 2) == 0 ? UL_ONEBITS : 0;
            if (fix_level)
                q |= fix_bit;
            else
                q &= ~fix_bit;
        } else if (io_bw == IO_BW_16) {
            q = (j % 2) == 0 ? (UL_ONEBITS & 0x0000ffff) : (0 & 0xffff0000);
            if (fix_level)
                q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            else
                q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
        } else {
            q = (j % 2) == 0 ? (UL_ONEBITS & 0x00ff00ff) : (0 & 0xff00ff00);
            if (fix_level)
                    q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) | ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
                else
                    q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) | ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
        }

        data[0] = data[2] = q;
        if (io_bw == IO_BW_32) {
            if (fix_level) {
                data[1] = data[3] = (~q) | fix_bit;
            } else {
                data[1] = data[3] = (~q) & (~fix_bit);
            }
        } else {
            data[1] = data[3] = q;
        }
        data_cpu_2_io(data, sizeof(data));
        printf("setting %3u", j);
        fflush(stdout);
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_checkerboard_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 q;
    u32 data[4];
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 64; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        if (io_bw == IO_BW_32) {
            q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
            if (fix_level)
                q |= fix_bit;
            else
                q &= ~fix_bit;
        } else if (io_bw == IO_BW_16) {
            q = ((CHECKERBOARD1 & 0x0000ffff) | (CHECKERBOARD2 & 0xffff0000));
            if (fix_level)
                q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            else
                q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
        } else {
            q = ((CHECKERBOARD1 & 0x00ff00ff) | (CHECKERBOARD2 & 0xff00ff00));
            if (fix_level)
                q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) | ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
            else
                q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) | ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
        }
        data[0] = data[2] = q;
        if (io_bw == IO_BW_32) {
            if (fix_level) {
                data[1] = data[3] = (~q) | fix_bit;
            } else {
                data[1] = data[3] = (~q) & (~fix_bit);
            }
        } else {
            data[1] = data[3] = q;
        }
        data_cpu_2_io(data, sizeof(data));
        printf("setting %3u", j);
        fflush(stdout);
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_blockseq_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 data[4];
    u32 q;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < 256; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        q = (u32)UL_BYTE(j);

        if (io_bw == IO_BW_32) {
            if (fix_level)
                q |= fix_bit;
            else
                q &= ~fix_bit;
        } else if (io_bw == IO_BW_16) {
            if (fix_level)
                q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            else
                q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
        } else {
            if (fix_level)
                q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) | ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
            else
                q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) | ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
        }

        data[0] = q;
        data[1] = q;
        data[2] = q;
        data[3] = q;
        data_cpu_2_io(data, sizeof(data));
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_walkbits0_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 data[4];
    u32 q;
    size_t i;

    printf("           ");
    fflush(stdout);
    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        printf("setting %3u", j);
        fflush(stdout);
        if (j < UL_LEN)
            q = ONE << j;
        else
            q = ONE << (UL_LEN * 2 - j - 1);

        if (io_bw == IO_BW_32) {
            if (fix_level)
                q |= fix_bit;
            else
                q &= ~fix_bit;
        } else if (io_bw == IO_BW_16) {
            if (fix_level)
                q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            else
                q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
        } else {
            if (fix_level)
                q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) | ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
            else
                q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) | ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
        }

        data[0] = q;
        data[1] = q;
        data[2] = q;
        data[3] = q;

        data_cpu_2_io(data, sizeof(data));

        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_walkbits1_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 data[4];
    u32 q;
    size_t i;

    printf("           ");
    fflush(stdout);

    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        printf("setting %3u", j);

        if (j < UL_LEN)
            q = UL_ONEBITS ^ (ONE << j);
        else
            q = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));

        if (io_bw == IO_BW_32) {
            if (fix_level)
                q |= fix_bit;
            else
                q &= ~fix_bit;
        } else if (io_bw == IO_BW_16) {
            if (fix_level)
                q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            else
                q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
        } else {
            if (fix_level)
                q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) | ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
            else
                q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) | ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
        }

        data[0] = q;
        data[1] = q;
        data[2] = q;
        data[3] = q;

        data_cpu_2_io(data, sizeof(data));

        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_bitspread_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j;
    u32 data[4];
    size_t i;
    u32 length;

    printf("           ");
    fflush(stdout);

    if (io_bw == IO_BW_32) {
        length = UL_LEN;
    } else if (io_bw == IO_BW_16) {
        length = UL_LEN / 2;
    } else {
        length = UL_LEN / 4;
    }

    for (j = 0; j < UL_LEN * 2; j++) {
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        p1 = (u32v *) bufa;
        p2 = (u32v *) bufb;
        printf("setting %3u", j);

        if (j < length) {
            data[0] = (ONE << j) | (ONE << (j + 2));
            data[1] = UL_ONEBITS ^ ((ONE << j) | (ONE << (j + 2)));
        } else {
            data[0] = (ONE << (length * 2 - 1 - j)) |
                      (ONE << (length * 2 + 1 - j));
            data[1] = UL_ONEBITS ^ (ONE << (length * 2 - 1 - j)
                                    | (ONE << (length * 2 + 1 - j)));
        }
        if (io_bw == IO_BW_16) {
            data[1] = data[0] = (data[0] & 0x0000ffff) | ((data[1] & 0x0000ffff) << 16);
        } else if (io_bw == IO_BW_8) {
            data[1] = data[0] = (data[0] & 0x000000ff) | ((data[1] & 0x000000ff) << 8) |
                                ((data[0] & 0x000000ff) << 16) | ((data[1] & 0x000000ff) << 24);
        }
        if (fix_level) {
            if (io_bw == IO_BW_32) {
                data[0] |= fix_bit;
                data[1] |= fix_bit;
            } else if (io_bw == IO_BW_16) {
                data[0] |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
                data[1] |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
            } else {
                data[0] |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) |
                        ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
                data[1] |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) |
                        ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
            }
        } else {
            if (io_bw == IO_BW_32) {
                data[0] &= ~fix_bit;
                data[1] &= ~fix_bit;
            } else if (io_bw == IO_BW_16) {
                data[0] &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
                data[1] &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
            } else {
                data[0] &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) |
                        ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
                data[1] &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) |
                        ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
            }
        }
        data[2] = data[0];
        data[3] = data[1];
        data_cpu_2_io(data, sizeof(data));

        fflush(stdout);
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = data[i & 3];
        }
        printf("\b\b\b\b\b\b\b\b\b\b\b");
        printf("testing %3u", j);
        fflush(stdout);
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

int test_bitflip_comparison(u32v *bufa, u32v *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u32v *p1 = bufa;
    u32v *p2 = bufb;
    unsigned int j, k;
    u32 q;
    u32 data[4];
    size_t i;

    printf("           ");
    fflush(stdout);
    for (k = 0; k < UL_LEN; k++) {
        if (io_bw == IO_BW_32)
            q = ONE << k;
        else if (io_bw == IO_BW_16)
            q = ONE << (k % 16);
        else
            q = ONE << (k % 8);
        for (j = 0; j < 8; j++) {
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            if (io_bw == IO_BW_32) {
                if (fix_level) {
                    q = (~q) | fix_bit;
                } else {
                    q = (~q) & (~fix_bit);
                }
            } else if (io_bw == IO_BW_16) {
                q = (q & 0x0000ffff) | ((~q & 0x0000ffff) << 16);
                if (fix_level)
                    q |= (fix_bit & 0x0000ffff) | ((fix_bit & 0x0000ffff) << 16);
                else
                    q &= (~fix_bit & 0x0000ffff) | ((~fix_bit & 0x0000ffff) << 16);
            } else {
                q = (q & 0x000000ff) | ((~q & 0x000000ff) << 8) |
                    ((q & 0x000000ff) << 16) | ((~q & 0x000000ff) << 24);
                if (fix_level)
                    q |= (fix_bit & 0x000000ff) | ((fix_bit & 0x000000ff) << 8) |
                         ((fix_bit & 0x000000ff) << 16) | ((fix_bit & 0x000000ff) << 24);
                else
                    q &= (~fix_bit & 0x000000ff) | ((~fix_bit & 0x000000ff) << 8) |
                         ((~fix_bit & 0x000000ff) << 16) | ((~fix_bit & 0x000000ff) << 24);
            }
            printf("setting %3u", k * 8 + j);
            fflush(stdout);
            data[0] = data[2] = q;
            if (io_bw == IO_BW_32) {
                if (fix_level) {
                    data[1] = data[3] = (~q) | fix_bit;
                } else {
                    data[1] = data[3] = (~q) & (~fix_bit);
                }
            } else {
                data[1] = data[3] = q;
            }
            data_cpu_2_io(data, sizeof(data));
            p1 = (u32v *) bufa;
            p2 = (u32v *) bufb;
            for (i = 0; i < count; i++) {
                *p1++ = *p2++ = data[i & 3];
            }
            printf("\b\b\b\b\b\b\b\b\b\b\b");
            printf("testing %3u", k * 8 + j);
            fflush(stdout);
            if (compare_regions(bufa, bufb, count)) {
                return -1;
            }
        }
    }
    printf("\b\b\b\b\b\b\b\b\b\b\b           \b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    return 0;
}

#ifdef TEST_NARROW_WRITES
int test_8bit_wide_random(u32v* bufa, u32v* bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u8v *p1, *t;
    u32v *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    putchar(' ');
    fflush(stdout);
    for (attempt = 0; attempt < 2;  attempt++) {
        if (attempt & 1) {
            p1 = (u8v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b=0; b < UL_LEN/8; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                putchar('\b');
                putchar(progress[++j % PROGRESSLEN]);
                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    fflush(stdout);
    return 0;
}

int test_16bit_wide_random(u32v* bufa, u32v* bufb, size_t count, unsigned int fix_bit, unsigned int fix_level) {
    u16v *p1, *t;
    u32v *p2;
    int attempt;
    unsigned int b, j = 0;
    size_t i;

    putchar( ' ' );
    fflush( stdout );
    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *) bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *) bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++) {
                *p1++ = *t++;
            }
            if (!(i % PROGRESSOFTEN)) {
                putchar('\b');
                putchar(progress[++j % PROGRESSLEN]);
                fflush(stdout);
            }
        }
        if (compare_regions(bufa, bufb, count)) {
            return -1;
        }
    }
    printf("\b \b");
    fflush(stdout);
    return 0;
}
#endif

void exit_result(int exit_code)
{
	printf("\n*************************************************************\n");
	printf("memtester result:\n");
	printf("Log: had found %u failures.\n", fault_cnt);
	printf("\nStatus: ");
	if (!fault_cnt) {
		printf("PASS.\n");
	} else {
		printf("FAIL - ");
		switch (exit_code) {
			EXIT_FAIL_NONSTARTER:
				printf("EXIT_FAIL_NONSTARTER\n");
				break;
			EXIT_FAIL_ADDRESSLINES:
				printf("EXIT_FAIL_ADDRESSLINES\n");
				break;
			EXIT_FAIL_OTHERTEST:
			default:
				printf("EXIT_FAIL_OTHERTEST\n");
				break;
		}
	}
	printf("\n*************************************************************\n");
	exit(exit_code);
}

