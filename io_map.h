/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _CMD_MEMTESTER_IO_MAP_H
#define _CMD_MEMTESTER_IO_MAP_H

#define IO_BW_32		0
#define IO_BW_16		1
#define IO_BW_8			2

extern u32 io_bw;

int data_cpu_2_io(void *p, u32 len);
void data_cpu_2_io_init(char *chip, char *ddr_bw);

#endif /* _CMD_MEMTESTER_IO_MAP_H */
