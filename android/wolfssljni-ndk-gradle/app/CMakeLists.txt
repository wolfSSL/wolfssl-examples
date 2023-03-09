# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

# Define project as both C and ASM for cases for SP has assembly enabled
project("wolfssljni-ndk-gradle" C ASM)

# set wolfSSL JNI location as environment variable, change if needed
set(wolfssljni_DIR ${CMAKE_SOURCE_DIR}/../wolfssljni)
set(wolfssl_DIR    ${CMAKE_SOURCE_DIR}/../wolfssl)

# ------------------------- wolfSSL Normal vs. FIPS Ready Selection --------------------------------
# Select if wolfSSL is normal ("normal") or FIPS Ready ("fipsready")
# wolfSSL FIPS Ready is available for download on the wolfssl.com download page. For more
# information on wolfSSL FIPS Ready see: https://www.wolfssl.com/license/fips/
#
# FIPS Ready requires different preprocessor flags, files, and a specific file ordering
# as set up below.
set(WOLFSSL_PKG_TYPE "normal")

# -------------------------------- Math Library Selection ------------------------------------------
# wolfCrypt math library selection, used to switch on below. Should be one of:
#     fastmath
#     spmath
set(WOLFSSL_MATH_LIB "spmath")

# --------------------------------- Include Directories --------------------------------------------
# Add header directories to include paths
include_directories(
        ${wolfssl_DIR}
        ${wolfssljni_DIR}/native
)

# ------------------------------- Preprocessor Defines ---------------------------------------------

if ("${WOLFSSL_PKG_TYPE}" MATCHES "normal")
    # Add preprocessor defines to CFLAGS, these match those placed into
    # wolfssl/options.h by configure if using: "./configure --enable-jni".
    # This list may be configurable depending on use case and desired optimizations.
    add_definitions(-DWC_RSA_BLINDING -DWOLFSSL_SHA224 -DWOLFSSL_SHA384
        -DWOLFSSL_SHA512 -DHAVE_HKDF -DNO_DSA -DHAVE_ECC
        -DECC_SHAMIR -DWC_RSA_PSS -DWOLFSSL_BASE64_ENCODE
        -DWOLFSSL_SHA3 -DHAVE_POLY1305 -DHAVE_CHACHA -DHAVE_HASHDRBG
        -DHAVE_TLS_EXTENSIONS -DHAVE_SUPPORTED_CURVES -DHAVE_FFDHE_2048
        -DWOLFSSL_TLS13 -DHAVE_EXTENDED_MASTER -DWOLFSSL_JNI
        -DHAVE_EX_DATA -DWOLFSSL_DTLS -DOPENSSL_EXTRA -DOPENSSL_ALL
        -DHAVE_CRL -DHAVE_OCSP -DHAVE_CRL_MONITOR
        -DPERSIST_SESSION_CACHE -DPERSIST_CERT_CACHE -DATOMIC_USER
        -DHAVE_PK_CALLBACKS -DWOLFSSL_CERT_EXT -DWOLFSSL_CERT_GEN
        -DHAVE_SNI -DHAVE_ALPN -DNO_RC4 -DHAVE_ENCRYPT_THEN_MAC
        -DNO_MD4 -DWOLFSSL_ENCRYPTED_KEYS -DHAVE_DH_DEFAULT_PARAMS
        -DNO_ERROR_QUEUE -DWOLFSSL_EITHER_SIDE -DWC_RSA_NO_PADDING
        -DWC_RSA_PSS -DWOLFSSL_PSS_LONG_SALT -DWOLFSSL_TICKET_HAVE_ID
        -DWOLFSSL_ERROR_CODE_OPENSSL -DWOLFSSL_ALWAYS_VERIFY_CB
        -DWOLFSSL_VERIFY_CB_ALL_CERTS -DWOLFSSL_EXTRA_ALERTS
        -DHAVE_EXT_CACHE -DWOLFSSL_FORCE_CACHE_ON_TICKET
        -DWOLFSSL_AKID_NAME -DHAVE_CTS -DNO_DES3 -DGCM_TABLE_4BIT
        -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT
        -DHAVE_AESGCM -DSIZEOF_LONG=4 -DSIZEOF_LONG_LONG=8

        # Defines added for wolfCrypt test and benchmark only, may not be needed for your
        # own application.
        -DUSE_CERT_BUFFERS_2048 -DUSE_CERT_BUFFERS_256 -DNO_WRITE_TEMP_FILES
        -DNO_FILESYSTEM -DNO_MAIN_DRIVER

        # Defines added for debugging. These can be removed if debug logging is not needed
        # and will increase performance and reduce library footprint size if removed.
        -DEBUG_WOLFSSL)

elseif("${WOLFSSL_PKG_TYPE}" MATCHES "fipsready")
    # Add preprocessor defines to CFLAGS, these match those placed into
    # wolfssl/options.h by configure if using: "./configure" on a Unix/Linux platform. The options
    # below have been chosen to match a FIPS Ready build, and are based on the example
    # user_settings.h file located here:
    # https://github.com/wolfSSL/wolfssl/blob/master/examples/configs/user_settings_fipsv5.h
    # This list may be configurable depending on use case and desired optimizations, being careful
    # not to break FIPS compatibility if targeting FIPS proper in the future.
    add_definitions(-DHAVE_FIPS -DHAVE_FIPS_VERSION=5 -DHAVE_FIPS_VERSION_MINOR=3
            -DHAVE_HASHDRBG -DHAVE_THREAD_LS -DHAVE_REPRODUCIBLE_BUILD
            -DFP_MAX_BITS=16384 -DSP_INT_BITS=8192 -DWOLFSSL_PUBLIC_MP
            -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT -DWC_RSA_BLINDING
            -DWC_RNG_SEED_CB -DWOLFSSL_VALIDATE_ECC_IMPORT -DWOLFSSL_VALIDATE_ECC_KEYGEN
            -DWOLFSSL_VALIDATE_FFC_IMPORT
            -DWOLFSSL_TLS13 -DHAVE_TLS_EXTENSIONS -DHAVE_ENCRYPT_THEN_MAC
            -DHAVE_SUPPORTED_CURVES -DHAVE_EXTENDED_MASTER -DHAVE_ONE_TIME_AUTH
            -DHAVE_SECURE_RENEGOTIATION -DHAVE_SERVER_RENEGOTIATION_INFO -DHAVE_SESSION_TICKET
            -DWOLFSSL_USE_ALIGN -DWOLFSSL_BASE64_ENCODE -DHAVE_CRL
            -DHAVE_EXT_CACHE -DWOLFSSL_VERIFY_CB_ALL_CERTS -DWOLFSSL_ALWAYS_VERIFY_CB
            -DWOLFSSL_DH_EXTRA -DWOLFSSL_WOLFSSH -DOPENSSL_EXTRA -DOPENSSL_ALL
            -DHAVE_FFDHE_Q -DHAVE_FFDHE_2048 -DHAVE_FFDHE_3072 -DHAVE_FFDHE_4096
            -DHAVE_FFDHE_6144 -DHAVE_FFDHE_8192 -DHAVE_DH_DEFAULT_PARAMS  -DHAVE_PUBLIC_FFDHE
            -DHAVE_ECC -DTFM_ECC256 -DECC_SHAMIR -DHAVE_ECC_CDH -DECC_USER_CURVES
            -DHAVE_ECC256 -DHAVE_ECC384 -DHAVE_ECC521 -DWOLFSSL_ECDSA_SET_K
            -DWC_RSA_PSS -DWOLFSSL_KEY_GEN -DWC_RSA_NO_PADDING
            -DWOLFSSL_AES_COUNTER -DHAVE_AESCCM -DHAVE_AES_ECB -DWOLFSSL_AES_DIRECT
            -DWOLFSSL_AES_OFB -DHAVE_AESGCM -DGCM_TABLE_4BIT -DWOLFSSL_CMAC
            -DWOLFSSL_SHA224 -DWOLFSSL_SHA512 -DWOLFSSL_SHA384 -DWOLFSSL_NO_SHAKE256
            -DWOLFSSL_NOSHA512_224 -DWOLFSSL_NOSHA512_256 -DWOLFSSL_SHA3 -DHAVE_HKDF
            -DNO_OLD_TLS -DNO_PSK -DNO_DO178
            -DNO_RC4 -DNO_MD4 -DNO_MD5 -DNO_DES3 -DNO_DSA -DNO_RABBIT

            -DWOLFSSL_JNI -DHAVE_EX_DATA -DHAVE_OCSP -DHAVE_CRL_MONITOR
            -DWOLFSSL_CERT_EXT -DWOLFSSL_CERT_GEN -DHAVE_SNI -DHAVE_ALPN
            -DWOLFSSL_ENCRYPTED_KEYS -DNO_ERROR_QUEUE -DWOLFSSL_EITHER_SIDE
            -DWOLFSSL_PSS_LONG_SALT -DWOLFSSL_TICKET_HAVE_ID
            -DWOLFSSL_ERROR_CODE_OPENSSL -DWOLFSSL_EXTRA_ALERTS
            -DWOLFSSL_FORCE_CACHE_ON_TICKET -DWOLFSSL_AKID_NAME -DHAVE_CTS
            -DKEEP_PEER_CERT -DSESSION_CERTS
            -DSIZEOF_LONG=4 -DSIZEOF_LONG_LONG=8

            # Enable below options for wolfSSL debug logging
            #-DDEBUG_WOLFSSL -DWOLFSSL_ANDROID_DEBUG

            # Below options are added only for wolfCrypt test and benchmark applications.
            # These can be left off / removed when integrating into a real-world application.
            -DNO_FILESYSTEM -DUSE_CERT_BUFFERS_2048 -DUSE_CERT_BUFFERS_256
            -DNO_WRITE_TEMP_FILES -DNO_MAIN_DRIVER)
endif()

if ("$WOLFSSL_MATH_LIB" MATCHES "fastmath")
    # Use fastmath library
    add_definitions(-DUSE_FAST_MATH -DTFM_ECC256 -DTFM_NO_ASM)

elseif("${WOLFSSL_MATH_LIB}" MATCHES "spmath")
    # Use SP math Library
    add_definitions(-DWOLFSSL_SP_MATH -DWOLFSSL_SP_MATH_ALL
            -DWOLFSSL_HAVE_SP_RSA -DWOLFSSL_SP_4096
            -DWOLFSSL_HAVE_SP_DH
            -DWOLFSSL_HAVE_SP_ECC -DWOLFSSL_SP_384 -DWOLFSSL_SP_521
            -DWOLFSSL_SP_LARGE_CODE)

    # SP Math architecture-specific settings (ex: assembly optimizations)
    if("${ANDROID_ABI}" MATCHES "arm64-v8a")
        add_definitions(-DWOLFSSL_SP_ASM -DWOLFSSL_SP_ARM64 -DWOLFSSL_SP_ARM64_ASM -DHAVE___UINT128_T)
    elseif("${ANDROID_ABI}" MATCHES "armeabi-v7a")
        # Add SP optimizations for ARMv7 here when available.
    elseif("${ANDROID_ABI}" MATCHES "x86_64")
        add_definitions(-DWOLFSSL_SP_ASM -DWOLFSSL_SP_X86_64 -DWOLFSSL_SP_X86_64_ASM -DHAVE___UINT128_T)
        list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_x86_64_asm.S)
    elseif("${ANDROID_ABI}" MATCHES "x86")
        # Add SP optimizations for X86 here when available.
    endif()
endif()

# --------------------------- wolfSSL and wolfCrypt Source Files -----------------------------------
# Add TLS sources to TLS_SOURCES list and remove files that are included inline by other files
aux_source_directory(${wolfssl_DIR}/src TLS_SOURCES)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/bio.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/conf.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/pk.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/ssl_misc.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/x509.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/x509_str.c)

if ("${WOLFSSL_PKG_TYPE}" MATCHES "normal")
    # Add crypto sources to CRYPTO_SOURCES, remove files that are included inline by other files
    aux_source_directory(${wolfssl_DIR}/wolfcrypt/src CRYPTO_SOURCES)
    list(REMOVE_ITEM CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/evp.c)
    list(REMOVE_ITEM CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/misc.c)

elseif("${WOLFSSL_PKG_TYPE}" MATCHES "fipsready")
    # FIPS Ready needs to explicitly order files for in-core integrity check to work properly.
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/wolfcrypt_first.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/hmac.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/random.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sha256.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/kdf.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/rsa.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/ecc.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/aes.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sha.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sha512.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sha3.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/dh.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/cmac.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/fips.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/fips_test.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/wolfcrypt_last.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/hash.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/cpuid.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/logging.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/wc_port.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/error.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/wc_encrypt.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/signature.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_arm32.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_arm64.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_armthumb.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_c32.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_c64.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_cortexm.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_dsp32.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_int.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/sp_x86_64.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/wolfmath.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/memory.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/asn.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/coding.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/pwdbased.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/pkcs12.c)
    list(APPEND CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/tfm.c)
endif()

# wolfSSL will be compiled as a SHARED library
add_library(wolfssl SHARED
            ${CRYPTO_SOURCES}
            ${TLS_SOURCES}
           )

# wolfSSL JNI/JSSE library wil be compiled as SHARED library
# wolfSSL JNI Java files are tied into build in Module build.gradle file
add_library(wolfssljni SHARED
            ${wolfssljni_DIR}/native/com_wolfssl_wolfcrypt_ECC.c
            ${wolfssljni_DIR}/native/com_wolfssl_wolfcrypt_EccKey.c
            ${wolfssljni_DIR}/native/com_wolfssl_wolfcrypt_RSA.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSL.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSLCertificate.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSLCertManager.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSLContext.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSLSession.c
            ${wolfssljni_DIR}/native/com_wolfssl_WolfSSLX509StoreCtx.c
           )

# Native library to hold sources for wolfCrypt test application. These include the
# wolfCrypt test and benchmark applications from within the wolfSSL proper source directory.
add_library(jnisample SHARED
            ${CMAKE_SOURCE_DIR}/src/main/cpp/NativeHelper.c
            ${wolfssl_DIR}/wolfcrypt/test/test.c
            ${wolfssl_DIR}/wolfcrypt/benchmark/benchmark.c
           )

# Find the android log library, store into variable ${lib-log}
find_library(lib-log log)

# Link libwolfssl to android log library
target_link_libraries(
        wolfssl
        ${lib-log}
)

# Link libjnisample to android log library
target_link_libraries(
        jnisample
        ${lib-log}
)

# Link libjnisample to libwolfssl
target_link_libraries(
        jnisample
        wolfssl
)

# Link libwolfssljni to libwolfssl
target_link_libraries(
        wolfssljni
        wolfssl
)

# Link libjnisample to libwolfssljni and libwolfssl
target_link_libraries(
        jnisample
        wolfssljni
        wolfssl
)
