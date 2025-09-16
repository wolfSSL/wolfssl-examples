add_executable(testwolfcrypt
    ../src/test_main.c
    ${WOLFSSL_ROOT}/wolfcrypt/test/test.c
)

target_compile_definitions(testwolfcrypt PRIVATE
    WOLFSSL_USER_SETTINGS
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

pico_add_extra_outputs(testwolfcrypt)
