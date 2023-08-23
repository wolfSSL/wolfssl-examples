cmake_minimum_required(VERSION 3.13)

# Pull in Pico and FreeRTOS
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)
#include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)


if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wolf_pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")


# Initialize the SDK
pico_sdk_init()

include_directories(config)
include_directories(include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include)
include_directories(${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include)
include_directories(${PICO_SDK_PATH}/lib/lwip/src/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include)

set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
include_directories(${WOLFSSL_ROOT})

file(GLOB_RECURSE WOLFSSL_SRC
    "${WOLFSSL_ROOT}/src/*.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/*.c"
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
target_include_directories(wolfssl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    )
target_compile_definitions(wolfssl PRIVATE
    WOLFSSL_USER_SETTINGS
    WOLFSSL_SP_ARM_THUMB_ASM
)


add_executable(testwolfcrypt
    src/test_main.c
    src/blink.c
    ${WOLFSSL_ROOT}/wolfcrypt/test/test.c
)

pico_enable_stdio_usb(testwolfcrypt 1)
pico_enable_stdio_uart(testwolfcrypt 0)
target_compile_definitions(testwolfcrypt PRIVATE
    WOLFSSL_USER_SETTINGS
)

target_link_libraries(testwolfcrypt
    wolfssl
    pico_stdlib
    pico_cyw43_arch_none
    pico_rand
)
pico_add_extra_outputs(testwolfcrypt)


add_executable(benchmark
    src/bench_main.c
    src/blink.c
    ${WOLFSSL_ROOT}/wolfcrypt/benchmark/benchmark.c
)

pico_enable_stdio_usb(benchmark 1)
pico_enable_stdio_uart(benchmark 0)
target_compile_definitions(benchmark PRIVATE
    WOLFSSL_USER_SETTINGS
)

target_link_libraries(benchmark
    wolfssl
    pico_stdlib
    pico_cyw43_arch_none
    pico_rand
)
pico_add_extra_outputs(benchmark)


add_executable(Wifi
    src/blink.c
    src/wifi.c
    src/Wifi_main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(Wifi 1)
pico_enable_stdio_uart(Wifi 0)

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

pico_add_extra_outputs(Wifi)

add_executable(tcp_Client
    src/blink.c
    src/wifi.c
    src/tcp.c
    src/tcpClient_main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(tcp_Client 1)
pico_enable_stdio_uart(tcp_Client 0)

target_compile_definitions(tcp_Client PRIVATE
    WOLFSSL_USER_SETTINGS
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

pico_add_extra_outputs(tcp_Client)


add_executable(tls_Client
    src/blink.c
    src/wifi.c
    src/tcp.c
    src/tlsClient_main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(tls_Client 1)
pico_enable_stdio_uart(tls_Client 0)

target_compile_definitions(tls_Client PRIVATE
    WOLFSSL_USER_SETTINGS
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

pico_add_extra_outputs(tls_Client)
