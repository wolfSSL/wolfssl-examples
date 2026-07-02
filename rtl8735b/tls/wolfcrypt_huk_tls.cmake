# wolfCrypt AmebaPro2 (RTL8735B) HUK-in-TLS example -- RealTek FreeRTOS SDK wiring.
#
# Install at <SDK>/component/example/wolfcrypt_huk_tls/ and select with:
#   cmake .. -DEXAMPLE=wolfcrypt_huk_tls -DWOLFSSL_ROOT=/path/to/wolfssl ...
# (also copy main.c to <SDK>/project/realtek_amebapro2_v0_example/src/main.c)
#
# Adds the wolfCrypt sources + the wolfSSL TLS layer + the RealTek HUK port,
# this example's include path, and -DWOLFSSL_USER_SETTINGS to the SDK app build.

if(NOT DEFINED WOLFSSL_ROOT OR WOLFSSL_ROOT STREQUAL "")
    if(DEFINED ENV{WOLFSSL_ROOT})
        set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
    else()
        set(WOLFSSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../wolfssl)
    endif()
endif()
if(NOT EXISTS "${WOLFSSL_ROOT}/wolfcrypt/src/aes.c")
    message(FATAL_ERROR
        "WOLFSSL_ROOT='${WOLFSSL_ROOT}' is not a wolfSSL tree. "
        "Pass -DWOLFSSL_ROOT=/path/to/wolfssl.")
endif()
message(STATUS "wolfCrypt HUK-TLS example: WOLFSSL_ROOT=${WOLFSSL_ROOT}")

### header search paths ###
list(APPEND app_example_inc_path
    ${WOLFSSL_ROOT}
    ${CMAKE_CURRENT_LIST_DIR}      # user_settings.h
)

### compile definitions (become -D...) ###
list(APPEND app_example_flags
    WOLFSSL_USER_SETTINGS
)

### source files ###
list(APPEND app_example_sources
    # --- wolfCrypt ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/aes.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha256.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha512.c        # SHA-384/512
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha.c           # SHA-1 (TLS sigalg paths)
    ${WOLFSSL_ROOT}/wolfcrypt/src/hash.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/hmac.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/kdf.c           # TLS PRF / HKDF
    ${WOLFSSL_ROOT}/wolfcrypt/src/random.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/memory.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_port.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/cryptocb.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/error.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/logging.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_encrypt.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/ecc.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/asn.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/coding.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sp_int.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wolfmath.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/port/realtek/rtl8735b.c
    # --- wolfSSL TLS layer ---
    ${WOLFSSL_ROOT}/src/internal.c
    ${WOLFSSL_ROOT}/src/keys.c
    ${WOLFSSL_ROOT}/src/tls.c
    ${WOLFSSL_ROOT}/src/tls13.c
    ${WOLFSSL_ROOT}/src/wolfio.c
    ${WOLFSSL_ROOT}/src/ssl.c
    ${WOLFSSL_ROOT}/src/ssl_load.c
    ${WOLFSSL_ROOT}/src/ssl_certman.c
    ${WOLFSSL_ROOT}/src/ssl_misc.c
    ${WOLFSSL_ROOT}/src/ssl_sess.c
    ${WOLFSSL_ROOT}/src/ssl_asn1.c
    ${WOLFSSL_ROOT}/src/ssl_crypto.c
    ${WOLFSSL_ROOT}/src/x509.c
    ${WOLFSSL_ROOT}/src/x509_str.c
)
