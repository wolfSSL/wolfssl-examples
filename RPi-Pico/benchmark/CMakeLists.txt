add_executable(benchmark
    ../src/bench_main.c
    ../src/blink.c
    ${WOLFSSL_ROOT}/wolfcrypt/benchmark/benchmark.c
)

target_link_libraries(benchmark
    wolfssl
    pico_stdlib
    pico_cyw43_arch_none
    pico_rand
)

target_compile_definitions(benchmark PRIVATE
    WOLFSSL_RPI_PICO
)

pico_enable_stdio_usb(benchmark 1)
pico_enable_stdio_uart(benchmark 1)

pico_add_extra_outputs(benchmark)