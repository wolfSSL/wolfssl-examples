# Makefile for memory bucket optimizer examples
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
CFLAGS = -Wall -Wextra -I../../wolfssl
LDFLAGS = -L../../wolfssl -lwolfssl

# Directories
SRC_DIR = src
EXAMPLE_DIR = examples
VISUALIZATION_DIR = visualization

# Check if wolfSSL is built with static memory
STATIC_MEM_CHECK = $(shell grep WOLFSSL_STATIC_MEMORY ../../wolfssl/options.h 2>/dev/null || echo "no")

# Check if gnuplot is available
GNUPLOT_CHECK = $(shell which gnuplot 2>/dev/null)

# Static analysis with cppcheck
CPPCHECK = cppcheck
CPPCHECK_FLAGS = --enable=all --suppress=missingIncludeSystem

.PHONY: all clean check optimizer examples visualize

all: optimizer examples
	@if [ -n "$(GNUPLOT_CHECK)" ]; then \
		$(MAKE) visualize; \
	else \
		echo "Warning: gnuplot not found, skipping visualization"; \
	fi

optimizer:
	$(MAKE) -C $(SRC_DIR)

examples:
	$(MAKE) -C $(EXAMPLE_DIR)

visualize:
	@if [ -n "$(GNUPLOT_CHECK)" ]; then \
		cd $(VISUALIZATION_DIR) && ./generate_data.sh || echo "Warning: Failed to generate visualization data"; \
	else \
		echo "Warning: gnuplot not found, skipping visualization"; \
	fi

check:
	$(CPPCHECK) $(CPPCHECK_FLAGS) $(SRC_DIR)/*.c $(EXAMPLE_DIR)/*.c

clean:
	$(MAKE) -C $(SRC_DIR) clean
	$(MAKE) -C $(EXAMPLE_DIR) clean
	rm -f $(VISUALIZATION_DIR)/*.png $(VISUALIZATION_DIR)/*.txt
	rm -rf results verification_results

# Print warning if static memory is not enabled
ifneq ($(STATIC_MEM_CHECK),yes)
$(warning wolfSSL is not built with static memory support)
$(warning Please rebuild wolfSSL with --enable-staticmemory to use the optimized configurations)
endif
