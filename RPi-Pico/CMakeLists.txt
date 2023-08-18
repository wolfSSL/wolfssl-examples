cmake_minimum_required(VERSION 3.13)

# Pull in Pico and FreeRTOS
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)


if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wolf_pico_examples)
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
