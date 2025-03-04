# Makefile for memory_bucket_optimizer
#
# Copyright (C) 2006-2025 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# wolfSSL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# wolfSSL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA

CC = gcc
CFLAGS = -Wall -Wextra -I../../../wolfssl
LDFLAGS = -L../../../wolfssl -lwolfssl

# Check if wolfSSL is built with static memory
STATIC_MEM_CHECK = $(shell grep WOLFSSL_STATIC_MEMORY ../../../wolfssl/options.h 2>/dev/null || echo "no")

all: memory_bucket_optimizer

memory_bucket_optimizer: memory_bucket_optimizer.c
	$(CC) $(CFLAGS) -o memory_bucket_optimizer memory_bucket_optimizer.c $(LDFLAGS)
	@if [ "$(STATIC_MEM_CHECK)" = "no" ]; then \
		echo "Warning: wolfSSL is not built with static memory support."; \
		echo "Please rebuild wolfSSL with --enable-staticmemory to use the optimized configurations."; \
	fi

clean:
	rm -f memory_bucket_optimizer

.PHONY: all clean
