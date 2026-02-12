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
    PICO_STACK_SIZE=4096
)

if (USE_UART)
pico_enable_stdio_usb(benchmark 0)
pico_enable_stdio_uart(benchmark 1)
else()
pico_enable_stdio_usb(benchmark 1)
pico_enable_stdio_uart(benchmark 0)
endif()

pico_add_extra_outputs(benchmark)
