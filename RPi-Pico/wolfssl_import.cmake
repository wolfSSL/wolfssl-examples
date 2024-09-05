
set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})

# ## wolfSSL/wolfCrypt library
file(GLOB WOLFSSL_SRC
    "${WOLFSSL_ROOT}/src/*.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/*.c"
)

file(GLOB WOLFSSL_EXCLUDE
    "${WOLFSSL_ROOT}/src/bio.c"
    "${WOLFSSL_ROOT}/src/conf.c"
    "${WOLFSSL_ROOT}/src/pk.c"
    "${WOLFSSL_ROOT}/src/x509.c"
    "${WOLFSSL_ROOT}/src/ssl_*.c"
    "${WOLFSSL_ROOT}/src/x509_*.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/misc.c"
    "${WOLFSSL_ROOT}/wolfcrypt/src/evp.c"
)

foreach(WOLFSSL_EXCLUDE ${WOLFSSL_EXCLUDE})
    list(REMOVE_ITEM WOLFSSL_SRC ${WOLFSSL_EXCLUDE})
endforeach()

add_library(wolfssl STATIC
    ${WOLFSSL_SRC}
)

include_directories(${WOLFSSL_ROOT})

target_compile_definitions(wolfssl PUBLIC
    WOLFSSL_USER_SETTINGS
)
### End of wolfSSL/wolfCrypt library
