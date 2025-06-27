/* rh_string.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */
#ifndef PORT_RH_STRING_H_
#define PORT_RH_STRING_H_

/* Use the system declarations */
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

/* Provide a limit of the printf format string to limit execution time */
#define MAX_FMT_LEN 64


/* Set up a user-provided circular buffer to capture printf output */
void rh_string_init(size_t buffer_size, size_t *offset, char* buffer);

/* Wrapper around printf that provides formatted hex output */
int snprintf(char* buffer, size_t bufsz, const char* format, ...);


#endif /* !PORT_RH_STRING_H*/