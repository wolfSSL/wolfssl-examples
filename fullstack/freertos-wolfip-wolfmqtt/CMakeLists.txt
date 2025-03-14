cmake_minimum_required(VERSION 3.13)
project(freertos_sim C)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# FreeRTOS source files
set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/freertos")
set(FREERTOS_KERNEL_DIR "${FREERTOS_DIR}/FreeRTOS-Kernel")

# wolfSSL directories
set(WOLFSSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../wolfssl")
set(WOLFIP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../wolfip")
set(WOLFMQTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../wolfMQTT")

# Certificate paths
set(WOLFSSL_CERTS_DIR "${WOLFSSL_DIR}/certs")

# Add definitions for certificate paths
add_definitions(
    -DMQTT_TLS_CA_CERT="${WOLFSSL_CERTS_DIR}/ca-cert.pem"
    -DMQTT_TLS_CLIENT_CERT="${WOLFSSL_CERTS_DIR}/client-cert.pem"
    -DMQTT_TLS_CLIENT_KEY="${WOLFSSL_CERTS_DIR}/client-key.pem"
)

# FreeRTOS configuration
add_definitions(
    -DPOSIX_PORT
    -DPOSIX
    -D_POSIX_C_SOURCE=200809L
    -D_POSIX_THREAD_SAFE_FUNCTIONS
    -DENABLE_MQTT_TLS
    -DDEBUG=0
)

# wolfSSL configuration
add_definitions(
    -DWOLFSSL_USER_SETTINGS
    -DHAVE_TLS_EXTENSIONS
    -DHAVE_SUPPORTED_CURVES
    -DTFM_TIMING_RESISTANT
    -DECC_TIMING_RESISTANT
    -DWC_RSA_BLINDING
    -DHAVE_AESGCM
    -DHAVE_CHACHA
    -DHAVE_POLY1305
    -DHAVE_ECC
    -DHAVE_CURVE25519
    -DHAVE_ED25519
    -DENABLE_MQTT_TLS
)

# wolfIP definitions
add_definitions(
    -DLINK_MTU=1500
    -DRXBUF_SIZE=8192
    -DTXBUF_SIZE=8192
    -DMAX_TCPSOCKETS=8
    -DMAX_UDPSOCKETS=8
    -DMAX_TIMERS=24
    -DWOLFIP_DEBUG
    -DWOLFSSL_DEBUG
    -DWOLFMQTT_DEBUG
    -DETH_TYPE_IP=0x0800
)

# FreeRTOS source files
set(FREERTOS_SRC
    ${FREERTOS_KERNEL_DIR}/tasks.c
    ${FREERTOS_KERNEL_DIR}/queue.c
    ${FREERTOS_KERNEL_DIR}/list.c
    ${FREERTOS_KERNEL_DIR}/timers.c
    ${FREERTOS_KERNEL_DIR}/event_groups.c
    ${FREERTOS_KERNEL_DIR}/stream_buffer.c
    ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix/port.c
    ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_3.c
    ${FREERTOS_DIR}/utils/posix_utils.c
)

# Create utils.c if it doesn't exist
if(NOT EXISTS "${FREERTOS_DIR}/utils/utils.c")
    file(MAKE_DIRECTORY "${FREERTOS_DIR}/utils")
    file(WRITE "${FREERTOS_DIR}/utils/utils.c" "/* utils.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Utility functions for FreeRTOS */
void utils_init(void)
{
    /* Initialize random seed */
    srand(time(NULL));
}
")
endif()

# Add utils.c to FreeRTOS sources
list(APPEND FREERTOS_SRC ${FREERTOS_DIR}/utils/utils.c)

# wolfIP source files - use stub implementation
set(WOLFIP_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wolfip_stub.c
)

# Application source files
set(APP_SRC
    src/main.c
    src/wolfip_freertos.c
    src/mqtt_client.c
    src/mqtt_net.c
    src/wolfip_utils.c
    src/wolfmqtt_stub.c
    src/tap_interface.c
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/FreeRTOSConfig
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FREERTOS_KERNEL_DIR}/include
    ${FREERTOS_KERNEL_DIR}/portable/ThirdParty/GCC/Posix
    ${WOLFSSL_DIR}
    ${WOLFIP_DIR}
    ${WOLFIP_DIR}/src/include
    ${WOLFMQTT_DIR}
)

# Create static library for wolfIP
add_library(wolfip STATIC ${WOLFIP_SRC})

# Set compiler flags for wolfIP library
target_compile_options(wolfip PRIVATE
    -Wall
    -Wextra
    -g
    -fpack-struct
)

# Create executable
add_executable(freertos_sim
    ${FREERTOS_SRC}
    ${APP_SRC}
)

# Link libraries
target_link_libraries(freertos_sim
    wolfip
    wolfssl
    pthread
    m
    rt
)

# Compiler flags
target_compile_options(freertos_sim PRIVATE
    -Wall
    -Wextra
    -g
    -fpack-struct
)
