add_executable(testwolfcrypt
    ../src/test_main.c
    ../src/blink.c
    ${WOLFSSL_ROOT}/wolfcrypt/test/test.c
)

target_link_libraries(testwolfcrypt
    wolfssl
    pico_stdlib
    pico_cyw43_arch_none
    pico_rand
)

pico_enable_stdio_usb(testwolfcrypt 1)
pico_enable_stdio_uart(testwolfcrypt 1)

pico_add_extra_outputs(testwolfcrypt)