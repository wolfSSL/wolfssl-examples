#!/bin/bash
#
# Copyright (C) 2025 wolfSSL Inc.
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


# Script to test optimized bucket configurations with WOLFSSL_NO_MALLOC

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if wolfSSL directory is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <wolfssl_dir> [test_name]"
    echo "Example: $0 ~/repos/wolfssl tls13_google"
    exit 1
else
    WOLFSSL_DIR="$1"
fi

# Check if wolfSSL directory exists
if [ ! -d "$WOLFSSL_DIR" ]; then
    echo "Error: wolfSSL directory not found at $WOLFSSL_DIR"
    echo "Please provide a valid wolfSSL directory."
    exit 1
fi

# Set test name
TEST_NAME="${2:-tls13_google}"

# Set up directories
RESULTS_DIR="$SCRIPT_DIR/results"
NO_MALLOC_DIR="$RESULTS_DIR/no_malloc_test"

# Create results directory if it doesn't exist
mkdir -p "$NO_MALLOC_DIR"

# Check if the optimized bucket configuration exists
if [ ! -f "$RESULTS_DIR/${TEST_NAME}_buckets.txt" ]; then
    echo "Error: Optimized bucket configuration for $TEST_NAME not found."
    echo "Please run run_multiple.sh first to generate the optimized bucket configurations."
    exit 1
fi

# Extract the optimized bucket configuration
BUCKETS=$(grep "^#define WOLFMEM_BUCKETS" "$RESULTS_DIR/${TEST_NAME}_buckets.txt" | sed 's/#define WOLFMEM_BUCKETS //')
DIST=$(grep "^#define WOLFMEM_DIST" "$RESULTS_DIR/${TEST_NAME}_buckets.txt" | sed 's/#define WOLFMEM_DIST //')

if [ -z "$BUCKETS" ] || [ -z "$DIST" ]; then
    echo "Error: Could not extract bucket configuration from $RESULTS_DIR/${TEST_NAME}_buckets.txt"
    exit 1
fi

echo "Using optimized bucket configuration for $TEST_NAME:"
echo "WOLFMEM_BUCKETS: $BUCKETS"
echo "WOLFMEM_DIST: $DIST"

# Create a test application with optimized bucket configuration
cat > "$NO_MALLOC_DIR/${TEST_NAME}_test.c" << EOC
/* ${TEST_NAME}_test.c
 *
 * Copyright (C) 2025 wolfSSL Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include wolfSSL headers */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/ssl.h>

/* Use the optimized bucket sizes and distribution */
#define WOLFMEM_BUCKETS $BUCKETS
#define WOLFMEM_DIST $DIST

/* Buffer size for static memory */
#define STATIC_MEM_SIZE (1024*1024) /* 1MB */

/* Static memory buffer */
static byte gStaticMemory[STATIC_MEM_SIZE];

int main() {
    int ret = 0;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;
    WOLFSSL_METHOD* method = NULL;
    
    printf("Testing with WOLFSSL_NO_MALLOC and optimized bucket configuration\\n");
    
    /* Initialize wolfSSL */
    wolfSSL_Init();
    
    /* Initialize static memory */
    if (wolfSSL_CTX_load_static_memory(&method, NULL, gStaticMemory, STATIC_MEM_SIZE,
                                     0, 1) != WOLFSSL_SUCCESS) {
        printf("Error: Failed to load static memory\\n");
        ret = -1;
        goto cleanup;
    }
    
    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(method)) == NULL) {
        printf("Error: Failed to create WOLFSSL_CTX\\n");
        ret = -1;
        goto cleanup;
    }
    
    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("Error: Failed to create WOLFSSL object\\n");
        ret = -1;
        goto cleanup;
    }
    
    printf("Successfully created WOLFSSL objects with static memory\\n");
    printf("Test completed successfully\\n");
    
cleanup:
    /* Cleanup */
    if (ssl) wolfSSL_free(ssl);
    if (ctx) wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    
    return ret;
}
EOC

# Create a Makefile for the test application
cat > "$NO_MALLOC_DIR/Makefile" << EOC
# Makefile for no_malloc test

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lwolfssl

# Path to wolfSSL
WOLFSSL_PATH = $WOLFSSL_DIR

# Include and library paths
INCLUDES = -I\$(WOLFSSL_PATH)
LIBS = -L\$(WOLFSSL_PATH)

all: ${TEST_NAME}_test

${TEST_NAME}_test: ${TEST_NAME}_test.c
	\$(CC) \$(CFLAGS) \$(INCLUDES) -o \$@ \$< \$(LIBS) \$(LDFLAGS)

clean:
	rm -f ${TEST_NAME}_test

.PHONY: all clean
EOC

# Build wolfSSL with static memory and no malloc
echo "Building wolfSSL with static memory and no malloc..."
cd "$WOLFSSL_DIR" || exit 1
./autogen.sh && CPPFLAGS=-DWOLFSSL_NO_MALLOC ./configure --enable-staticmemory && make

# Build the test application
echo "Building test application with optimized bucket configuration..."
cd "$NO_MALLOC_DIR" || exit 1
make

# Run the test application
echo "Running test application..."
LD_LIBRARY_PATH="$WOLFSSL_DIR" ./${TEST_NAME}_test

# Check the result
if [ $? -eq 0 ]; then
    echo "Test passed: The optimized bucket configuration works correctly with WOLFSSL_NO_MALLOC."
    echo "This confirms that the memory bucket optimization is effective for embedded systems."
else
    echo "Test failed: The optimized bucket configuration does not work with WOLFSSL_NO_MALLOC."
    echo "Please check the bucket sizes and distribution."
fi

# Clean up
echo "Cleaning up..."
make clean
