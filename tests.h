/*
 * Very simple yet very effective memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2024 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the declarations for the functions for the actual tests,
 * called from the main routine in memtester.c.  See other comments in that 
 * file.
 *
 */

/* Function declaration. */

int test_stuck_address(unsigned int volatile *bufa, size_t count);
int test_random_value(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_xor_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_sub_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_mul_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_div_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_or_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_and_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_seqinc_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_solidbits_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_checkerboard_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_blockseq_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_walkbits0_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_walkbits1_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_bitspread_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_bitflip_comparison(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
#ifdef TEST_NARROW_WRITES    
int test_8bit_wide_random(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
int test_16bit_wide_random(unsigned int volatile *bufa, unsigned int volatile *bufb, size_t count, unsigned int fix_bit, unsigned int fix_level);
#endif
void exit_result(int exit_code);

