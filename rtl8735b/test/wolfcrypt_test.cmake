# wolfCrypt AmebaPro2 (RTL8735B) test + benchmark -- RealTek FreeRTOS SDK wiring.
#
# Install at <SDK>/component/example/wolfcrypt_test/ and select at configure:
#   cmake .. -DEXAMPLE=wolfcrypt_test -DWOLFSSL_ROOT=/path/to/wolfssl \
#            -DRTL_BENCH_MODE=N ...
# (also copy main.c to <SDK>/project/realtek_amebapro2_v0_example/src/main.c)
#
# RTL_BENCH_MODE selects the crypto backend (matches user_settings.h):
#   1  pure C       generic sp_int.c + C symmetric/hash (baseline)
#   2  Thumb-2 / SP sp_cortexm.c + thumb2-*-asm.S (default)
#   3  RealTek HW   HUK crypto-callback over the silicon engine (sp_int.c base
#                   + cryptocb.c + the RealTek port); benchmark only.

# Locate the wolfSSL tree: -DWOLFSSL_ROOT=..., or $WOLFSSL_ROOT, else a guess.
if(NOT DEFINED WOLFSSL_ROOT OR WOLFSSL_ROOT STREQUAL "")
    if(DEFINED ENV{WOLFSSL_ROOT})
        set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
    else()
        set(WOLFSSL_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../wolfssl)
    endif()
endif()
if(NOT EXISTS ${WOLFSSL_ROOT}/wolfcrypt/src/aes.c)
    message(FATAL_ERROR
        "WOLFSSL_ROOT='${WOLFSSL_ROOT}' is not a wolfSSL tree. "
        "Pass -DWOLFSSL_ROOT=/path/to/wolfssl.")
endif()
if(NOT DEFINED RTL_BENCH_MODE OR RTL_BENCH_MODE STREQUAL "")
    set(RTL_BENCH_MODE 2)
endif()
message(STATUS "wolfcrypt_test example: WOLFSSL_ROOT=${WOLFSSL_ROOT} "
               "RTL_BENCH_MODE=${RTL_BENCH_MODE}")

### header search paths ###
list(APPEND app_example_inc_path
    ${WOLFSSL_ROOT}
    ${CMAKE_CURRENT_LIST_DIR}      # user_settings.h
)

### compile definitions (become -D...) ###
list(APPEND app_example_flags
    WOLFSSL_USER_SETTINGS
    RTL_BENCH_MODE=${RTL_BENCH_MODE}
)

### source files (backend-independent) ###
list(APPEND app_example_sources
    # --- symmetric ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/aes.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/cmac.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/chacha.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/chacha20_poly1305.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/poly1305.c
    # --- hashing ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha256.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha512.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/sha3.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/hash.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/hmac.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/kdf.c
    # --- public key ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/rsa.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/dh.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/ecc.c
    # --- math (generic single-precision always present) ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/sp_int.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wolfmath.c
    # --- encoding / support ---
    ${WOLFSSL_ROOT}/wolfcrypt/src/asn.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/coding.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/random.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/memory.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_port.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/wc_encrypt.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/error.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/logging.c
    ${WOLFSSL_ROOT}/wolfcrypt/src/signature.c
    # --- the test + benchmark drivers ---
    ${WOLFSSL_ROOT}/wolfcrypt/test/test.c
    ${WOLFSSL_ROOT}/wolfcrypt/benchmark/benchmark.c
)

### mode 2: Cortex-M / Thumb-2 assembly fast paths ###
if(RTL_BENCH_MODE EQUAL 2)
    list(APPEND app_example_sources
        ${WOLFSSL_ROOT}/wolfcrypt/src/sp_cortexm.c
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-aes-asm.S
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-chacha-asm.S
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-poly1305-asm.S
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-sha256-asm.S
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-sha512-asm.S
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/arm/thumb2-sha3-asm.S
    )
endif()

### mode 3: RealTek HUK crypto-callback device (silicon engine) ###
if(RTL_BENCH_MODE EQUAL 3)
    list(APPEND app_example_sources
        ${WOLFSSL_ROOT}/wolfcrypt/src/cryptocb.c
        ${WOLFSSL_ROOT}/wolfcrypt/src/port/realtek/rtl8735b.c
    )
endif()
