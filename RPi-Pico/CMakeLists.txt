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

# Initialize the SDK
pico_sdk_init()

message("Example cmake project including wolfSSL with user_settings.h")

# add global define to include user_settings.h
add_compile_definitions(WOLFSSL_USER_SETTINGS)
add_compile_definitions(WOLFSSL_USER_SETTINGS_ASM)
set(BUILD_SHARED_LIBS OFF)
set(WOLFSSL_EXAMPLES OFF)
set(WOLFSSL_CRYPT_TESTS OFF)
set(WOLFSSL_USER_SETTINGS ON)

include_directories(config)
include_directories(include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include)
include_directories(${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include)
include_directories(${PICO_SDK_PATH}/lib/lwip/src/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include)

add_subdirectory(wolfssl)
target_link_libraries(wolfssl PRIVATE
)

add_executable(benchmark
    src/bench_main.c
    src/blink.c
    wolfssl/wolfcrypt/benchmark/benchmark.c
)

pico_enable_stdio_usb(benchmark 1)
pico_enable_stdio_uart(benchmark 0)
target_compile_definitions(benchmark PRIVATE

)
target_link_libraries(benchmark
    wolfssl
    pico_stdlib
    pico_cyw43_arch_none
    pico_rand
    #FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
)
pico_add_extra_outputs(benchmark)

if(0)
    add_executable(TCPclient
        src/blink.c
        # src/TCPclient.c
        src/TCPclient_main.c
    )

    # enable usb output, disable uart output
    pico_enable_stdio_usb(TCPclient 1)
    pico_enable_stdio_uart(TCPclient 0)

    target_compile_definitions(TCPclient PRIVATE

        # CYW43_NETUTILS
        # PICO_CYW43_ARCH_FREERTOS
        TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        PICO_CYW43_ARCH_POLL
        NO_SYS=1

        # SYS_LIGHTWEIGHT_PROT=1
    )

    target_link_libraries(TCPclient
        pico_stdlib
        pico_rand

        # pico_cyw43_arch_lwip_sys_freertos
        # pico_cyw43_arch_lwip_poll
        # pico_cyw43_arch_none

        pico_lwip
        # pico_lwip_core4
        # pico_lwip_core6
        pico_cyw43_arch
        pico_lwip_nosys
        pico_async_context_poll
        # pico_lwip_iperf

        # pico_lwip_freertos
        # pico_async_context_freertos
        # pico_lwip_port
        # pico_cyw43_driver
        # pico_lwip_api
        pico_lwip_netif
        # FreeRTOS-Kernel
        # FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
    )

    pico_add_extra_outputs(TCPclient)
endif()

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