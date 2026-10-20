/* c2000_board.h
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Board bring-up shared by the standalone SHA-2 / SHA-3 examples
 * (Source/test_sha2.c, Source/test_sha3.c). */

#ifndef C2000_BOARD_H
#define C2000_BOARD_H

/* PLL to 150 MHz, GPIO, SCIA console (unless built with CIO=1) and the
 * free-running CPU Timer 0 used by current_time(). */
void c2000_board_init(void);

/* Elapsed seconds since the last current_time(1) call. */
double current_time(int reset);

#endif /* C2000_BOARD_H */
