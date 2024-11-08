cmake_minimum_required(VERSION 3.13)

# Pull in Pico and FreeRTOS
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#include(pico_extras_import_optional.cmake)
#include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)


if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wolf_pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

option(USE_UART "Output over UART instead of USB" OFF)
option(USE_WIFI "Enable WiFi" OFF)
option(WIFI_SSID "The WiFi SSID to connect to" "")
option(WIFI_PASSWORD "The WiFi password" "")
option(TEST_TCP_SERVER_IP "The TCP test server IP" "")

if (USE_WIFI AND NOT PICO_CYW43_SUPPORTED)
    message(FATAL_ERROR "You can only set USE_WIFI when a PICO_BOARD with wifi is used")
endif()

### Global Include Path
    include_directories(config)
    include_directories(include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/src/include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include)

    set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
    include_directories(${WOLFSSL_ROOT})
### End of Global Include Path


### wolfSSL/wolfCrypt library
    file(GLOB WOLFSSL_SRC
        "${WOLFSSL_ROOT}/src/*.c"
        "${WOLFSSL_ROOT}/wolfcrypt/src/*.c"
        "${WOLFSSL_ROOT}/wolfcrypt/src/port/rpi_pico/*"
    )
    list(REMOVE_ITEM WOLFSSL_SRC EXCLUDE REGEX
        "${WOLFSSL_ROOT}/src/bio.c"
        "${WOLFSSL_ROOT}/src/conf.c"
        "${WOLFSSL_ROOT}/src/pk.c"
        "${WOLFSSL_ROOT}/src/ssl_asn1.c"
        "${WOLFSSL_ROOT}/src/ssl_bn.c"
        "${WOLFSSL_ROOT}/src/ssl_misc.c"
        "${WOLFSSL_ROOT}/src/x509.c"
        "${WOLFSSL_ROOT}/src/x509_str.c"
        "${WOLFSSL_ROOT}/wolfcrypt/src/evp.c"
        "${WOLFSSL_ROOT}/wolfcrypt/src/misc.c"
    )

    add_library(wolfssl STATIC
        ${WOLFSSL_SRC}
    )

    target_compile_definitions(wolfssl PUBLIC
        WOLFSSL_USER_SETTINGS
    )
    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    target_link_libraries(wolfssl
        pico_stdlib
        pico_rand
    )
### End of wolfSSL/wolfCrypt library


### Test wolfCrypt algorithms
    add_executable(testwolfcrypt
        src/test_main.c
        ${WOLFSSL_ROOT}/wolfcrypt/test/test.c
    )

    target_link_libraries(testwolfcrypt
        wolfssl
        pico_stdlib
        pico_rand
    )

    if (USE_UART)
        pico_enable_stdio_usb(testwolfcrypt 0)
        pico_enable_stdio_uart(testwolfcrypt 1)
    else()
        pico_enable_stdio_usb(testwolfcrypt 1)
        pico_enable_stdio_uart(testwolfcrypt 0)
    endif()

    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    pico_add_extra_outputs(testwolfcrypt)
### End of Test wolfCrypt algorithms


### Benchmark wolfCrypt algorithms
    add_executable(benchmark
        src/bench_main.c
        ${WOLFSSL_ROOT}/wolfcrypt/benchmark/benchmark.c
    )

    target_link_libraries(benchmark
        wolfssl
        pico_stdlib
        pico_rand
    )

    if (USE_UART)
        pico_enable_stdio_usb(benchmark 0)
        pico_enable_stdio_uart(benchmark 1)
    else()
        pico_enable_stdio_usb(benchmark 1)
        pico_enable_stdio_uart(benchmark 0)
    endif()


    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    pico_add_extra_outputs(benchmark)
### End of Benchmark wolfCrypt algorithms


if (USE_WIFI)
### Wifi connection
    add_executable(Wifi
        src/blink.c
        src/wifi.c
        src/wifi_main.c
    )

    target_compile_definitions(Wifi PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        PICO_CYW43_ARCH_POLL
        NO_SYS=1
    )

    target_link_libraries(Wifi
        pico_stdlib
        pico_rand
        pico_lwip
        pico_cyw43_arch
        pico_lwip_nosys
        pico_async_context_poll
    )

    if (USE_UART)
        pico_enable_stdio_usb(Wifi 0)
        pico_enable_stdio_uart(Wifi 1)
    else()
        pico_enable_stdio_usb(Wifi 1)
        pico_enable_stdio_uart(Wifi 0)
    endif()

    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    pico_add_extra_outputs(Wifi)
### End of Wifi connection
endif()

if (USE_WIFI)
### TCP Client
    add_executable(tcp_Client
        src/blink.c
        src/wifi.c
        src/tcp.c
        src/tcpClient_main.c
    )

    target_compile_definitions(tcp_Client PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
        PICO_CYW43_ARCH_POLL
        NO_SYS=1
    )

    target_link_libraries(tcp_Client
        pico_stdlib
        pico_rand
        pico_lwip
        pico_cyw43_arch
        pico_lwip_nosys
        pico_async_context_poll
    )

    if (USE_UART)
        pico_enable_stdio_usb(tcp_Client 0)
        pico_enable_stdio_uart(tcp_Client 1)
    else()
        pico_enable_stdio_usb(tcp_Client 1)
        pico_enable_stdio_uart(tcp_Client 0)
    endif()

    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    pico_add_extra_outputs(tcp_Client)
### End of TCP Client
endif()

if (USE_WIFI)
### TLS Client
    add_executable(tls_Client
        src/blink.c
        src/wifi.c
        src/tcp.c
        src/tlsClient_main.c
    )

    target_compile_definitions(tls_Client PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
        PICO_CYW43_ARCH_POLL
        NO_SYS=1
    )

    target_link_libraries(tls_Client
        pico_stdlib
        pico_rand
        pico_lwip
        pico_cyw43_arch
        pico_lwip_nosys
        pico_async_context_poll
        wolfssl
    )

    if (USE_UART)
        pico_enable_stdio_usb(tls_Client 0)
        pico_enable_stdio_uart(tls_Client 1)
    else()
        pico_enable_stdio_usb(tls_Client 1)
        pico_enable_stdio_uart(tls_Client 0)
    endif()

    if (${PICO_PLATFORM} STREQUAL "rp2350")
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_CORTEX_M_ASM)
    elseif (${PICO_PLATFORM} STREQUAL "rp2350-riscv")
        add_compile_definitions(wolfSSL WOLFSSL_SP_RISCV32)
    else()
        add_compile_definitions(wolfssl WOLFSSL_SP_ARM_THUMB_ASM)
    endif()

    pico_add_extra_outputs(tls_Client)
### End of TLS Client
endif()
