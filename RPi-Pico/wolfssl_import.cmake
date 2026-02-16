
set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})

# ## wolfSSL/wolfCrypt library
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

include_directories(${WOLFSSL_ROOT})

target_compile_definitions(wolfssl PUBLIC
    WOLFSSL_USER_SETTINGS
)

add_compile_definitions(PICO_USE_STACK_GUARDS=1)

if (${PICO_PLATFORM} STREQUAL "rp2350-arm-s")
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
