# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

# Define project as both C and ASM for cases for SP has assembly enabled
project("wolfssljni-ndk-gradle" C ASM)

# set wolfSSL JNI location as environment variable, change if needed
set(wolfssljni_DIR ${CMAKE_SOURCE_DIR}/../wolfssljni)
set(wolfssl_DIR    ${CMAKE_SOURCE_DIR}/../wolfssl)

# Math library selection, used to switch on below. Should be one of:
#     fastmath
#     spmath
set(WOLFSSL_MATH_LIB "spmath")

# Add header directories to include paths
include_directories(
        ${wolfssl_DIR}
        ${wolfssljni_DIR}/native
)

# Add TLS sources to TLS_SOURCES list and remove files that are included inline by other files
aux_source_directory(${wolfssl_DIR}/src TLS_SOURCES)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/bio.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/conf.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/pk.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/x509.c)
list(REMOVE_ITEM TLS_SOURCES ${wolfssl_DIR}/src/x509_str.c)

# Add crypto sources to CRYPTO_SOURCES list and remove files that are included inline by other files
aux_source_directory(${wolfssl_DIR}/wolfcrypt/src CRYPTO_SOURCES)
list(REMOVE_ITEM CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/evp.c)
list(REMOVE_ITEM CRYPTO_SOURCES ${wolfssl_DIR}/wolfcrypt/src/misc.c)

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
