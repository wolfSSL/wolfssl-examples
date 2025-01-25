add_executable(testwolfcrypt
    ../src/test_main.c
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
