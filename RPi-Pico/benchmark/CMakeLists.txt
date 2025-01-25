add_executable(benchmark
    ../src/bench_main.c
    ${WOLFSSL_ROOT}/wolfcrypt/benchmark/benchmark.c
)

target_link_libraries(benchmark
    wolfssl
    pico_stdlib
    pico_rand
)

target_compile_definitions(benchmark PRIVATE
    WOLFSSL_RPI_PICO
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
