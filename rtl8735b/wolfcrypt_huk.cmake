# wolfCrypt AmebaPro2 (RTL8735B) HUK example -- RealTek FreeRTOS SDK wiring.
#
# Install this directory at <SDK>/component/example/wolfcrypt_huk/ and select it
# at configure time:
#   cmake .. -DEXAMPLE=wolfcrypt_huk -DWOLFSSL_ROOT=/path/to/wolfssl ...
# (also copy main.c to <SDK>/project/realtek_amebapro2_v0_example/src/main.c)
#
# Adds the wolfCrypt sources + the RealTek HUK crypto-callback port + this
# example's include path and -DWOLFSSL_USER_SETTINGS to the SDK app build.

# Locate the wolfSSL tree: -DWOLFSSL_ROOT=..., or $WOLFSSL_ROOT, else a guess.
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
message(STATUS "wolfCrypt HUK example: WOLFSSL_ROOT=${WOLFSSL_ROOT}")

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
    ${WOLFSSL_ROOT}/wolfcrypt/src/aes.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha256.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/hash.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/hmac.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/random.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/memory.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_port.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/cryptocb.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/error.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/logging.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_encrypt.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/port/realtek/rtl8735b.c
    # ECC / ECDSA P-256 (HUK sign demo) + its ASN.1 + SP big-integer math
    ${WOLFSSL_ROOT}/wolfcrypt/src/ecc.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/asn.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/coding.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sp_int.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wolfmath.c
)
