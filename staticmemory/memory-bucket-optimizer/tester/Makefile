# Makefile for memory bucket tester
#
# Copyright (C) 2025 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# wolfSSL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
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
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS = -Wall -Wextra -O2 -g
INCLUDES = -I$(WOLFSSL_INSTALL_DIR)/include
LIBS = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

TARGET = memory_bucket_tester
SOURCE = memory_bucket_tester.c

.PHONY: all clean test check

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)


check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: memory_bucket_tester
	./memory_bucket_tester mem-sample.log --buckets 64,128,256 --dist 1,1,1 --buffer-size 8192 | grep -q 'TEST PASSED: All allocations succeeded'
	@echo "PASS: staticmemory-bucket-tester checks"
