/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _VA_2_PA_H
#define _VA_2_PA_H

int data_cpu_2_io(void *p, u32 len);
void data_cpu_2_io_init(char *chip, char *ddr_bw);

unsigned long read_pagemap(unsigned long virt_addr);

#endif /* _VA_2_PA_H */
