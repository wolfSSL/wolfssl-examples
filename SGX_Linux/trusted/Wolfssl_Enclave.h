/* Wolfssl_Enclave.h
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _BENCHMARK_ENCLAVE_H_
#define _BENCHMARK_ENCLAVE_H_

#if defined(__cplusplus)
extern "C" {
#endif

void printf(const char *fmt, ...);
int sprintf(char* buf, const char *fmt, ...);
double current_time(void);

#if defined(__cplusplus)
}
#endif

#endif /* !_BENCHMARK_ENCLAVE_H_ */
