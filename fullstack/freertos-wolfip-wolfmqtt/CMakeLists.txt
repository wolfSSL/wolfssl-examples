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
    ${FREERTOS_DIR}/utils/utils.c
)

# wolfIP source files
set(WOLFIP_SRC
    ${WOLFIP_DIR}/src/wolfip.c
    ${WOLFIP_DIR}/src/fifo.c
    ${WOLFIP_DIR}/src/http/httpd.c
    ${WOLFIP_DIR}/src/port/wolfssl_io.c
    ${WOLFIP_DIR}/src/port/posix/linux_tap.c
    ${WOLFIP_DIR}/src/port/posix/bsd_socket.c
)

# Application source files
set(APP_SRC
    src/main.c
    src/wolfip_freertos.c
    src/mqtt_client.c
    src/mqtt_net.c
    src/wolfip_utils.c
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
    wolfmqtt
    pthread
    m
)

# Compiler flags
target_compile_options(freertos_sim PRIVATE
    -Wall
    -Wextra
    -g
    -fpack-struct
)
