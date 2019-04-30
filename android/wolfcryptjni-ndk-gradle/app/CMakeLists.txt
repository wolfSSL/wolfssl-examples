# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Add preprocessor defines to CFLAGS
add_definitions(-DNO_DSA -DNO_PSK -DNO_MD4 -DNO_HC128 -DNO_RABBIT
                -DWOLFSSL_SHA512 -DWOLFSSL_SHA384 -DHAVE_AESGCM -DHAVE_HASHDRBG
                -DHAVE_ECC -DWOLFSSL_KEY_GEN -DNO_PWDBASED -DUSE_FAST_MATH
                -DTFM_ECC256 -DHAVE_DH -DUSE_FAST_MATH -DTFM_TIMING_RESISTANT
                -DECC_TIMING_RESISTANT -DWC_RSA_BLINDING -DTFM_NO_ASM
                )

# set wolfSSL JNI location as environment variable, change if needed
set(wolfcryptjni_DIR ${CMAKE_SOURCE_DIR}/../wolfcrypt-jni)
set(wolfssl_DIR      ${CMAKE_SOURCE_DIR}/../wolfssl)

# Add header directories to include paths
include_directories(
            ${wolfssl_DIR}
            ${wolfcryptjni_DIR}/jni/include
            )

# Add wolfSSL library source files, to be compiled as SHARED library
add_library(wolfssl SHARED
            ${wolfssl_DIR}/wolfcrypt/src/aes.c
            ${wolfssl_DIR}/wolfcrypt/src/arc4.c
            ${wolfssl_DIR}/wolfcrypt/src/asm.c
            ${wolfssl_DIR}/wolfcrypt/src/asn.c
            ${wolfssl_DIR}/wolfcrypt/src/blake2b.c
            ${wolfssl_DIR}/wolfcrypt/src/camellia.c
            ${wolfssl_DIR}/wolfcrypt/src/chacha.c
            ${wolfssl_DIR}/wolfcrypt/src/chacha20_poly1305.c
            ${wolfssl_DIR}/wolfcrypt/src/cmac.c
            ${wolfssl_DIR}/wolfcrypt/src/coding.c
            ${wolfssl_DIR}/wolfcrypt/src/compress.c
            ${wolfssl_DIR}/wolfcrypt/src/cpuid.c
            ${wolfssl_DIR}/wolfcrypt/src/curve25519.c
            ${wolfssl_DIR}/wolfcrypt/src/des3.c
            ${wolfssl_DIR}/wolfcrypt/src/dh.c
            ${wolfssl_DIR}/wolfcrypt/src/dsa.c
            ${wolfssl_DIR}/wolfcrypt/src/ecc.c
            ${wolfssl_DIR}/wolfcrypt/src/ecc_fp.c
            ${wolfssl_DIR}/wolfcrypt/src/ed25519.c
            ${wolfssl_DIR}/wolfcrypt/src/error.c
            ${wolfssl_DIR}/wolfcrypt/src/fe_low_mem.c
            ${wolfssl_DIR}/wolfcrypt/src/fe_operations.c
            ${wolfssl_DIR}/wolfcrypt/src/ge_low_mem.c
            ${wolfssl_DIR}/wolfcrypt/src/ge_operations.c
            ${wolfssl_DIR}/wolfcrypt/src/hash.c
            ${wolfssl_DIR}/wolfcrypt/src/hc128.c
            ${wolfssl_DIR}/wolfcrypt/src/hmac.c
            ${wolfssl_DIR}/wolfcrypt/src/idea.c
            ${wolfssl_DIR}/wolfcrypt/src/integer.c
            ${wolfssl_DIR}/wolfcrypt/src/logging.c
            ${wolfssl_DIR}/wolfcrypt/src/md2.c
            ${wolfssl_DIR}/wolfcrypt/src/md4.c
            ${wolfssl_DIR}/wolfcrypt/src/md5.c
            ${wolfssl_DIR}/wolfcrypt/src/memory.c
            ${wolfssl_DIR}/wolfcrypt/src/pkcs12.c
            ${wolfssl_DIR}/wolfcrypt/src/pkcs7.c
            ${wolfssl_DIR}/wolfcrypt/src/poly1305.c
            ${wolfssl_DIR}/wolfcrypt/src/pwdbased.c
            ${wolfssl_DIR}/wolfcrypt/src/rabbit.c
            ${wolfssl_DIR}/wolfcrypt/src/random.c
            ${wolfssl_DIR}/wolfcrypt/src/ripemd.c
            ${wolfssl_DIR}/wolfcrypt/src/rsa.c
            ${wolfssl_DIR}/wolfcrypt/src/sha.c
            ${wolfssl_DIR}/wolfcrypt/src/sha256.c
            ${wolfssl_DIR}/wolfcrypt/src/sha3.c
            ${wolfssl_DIR}/wolfcrypt/src/sha512.c
            ${wolfssl_DIR}/wolfcrypt/src/signature.c
            ${wolfssl_DIR}/wolfcrypt/src/srp.c
            ${wolfssl_DIR}/wolfcrypt/src/tfm.c
            ${wolfssl_DIR}/wolfcrypt/src/wc_encrypt.c
            ${wolfssl_DIR}/wolfcrypt/src/wc_port.c
            ${wolfssl_DIR}/wolfcrypt/src/wolfevent.c
            ${wolfssl_DIR}/wolfcrypt/src/wolfmath.c
            ${wolfssl_DIR}/src/crl.c
            ${wolfssl_DIR}/src/internal.c
            ${wolfssl_DIR}/src/wolfio.c
            ${wolfssl_DIR}/src/keys.c
            ${wolfssl_DIR}/src/ocsp.c
            ${wolfssl_DIR}/src/sniffer.c
            ${wolfssl_DIR}/src/ssl.c
            ${wolfssl_DIR}/src/tls.c
            ${wolfssl_DIR}/src/tls13.c
           )

# Add wolfCrypt JNI library native source files, to be compiled as SHARED
# library. wolfCrypt JNI Java files are tied into build in Module build.gradle
# file
add_library(wolfcryptjni SHARED
            ${wolfcryptjni_DIR}/jni/jni_aes.c
            ${wolfcryptjni_DIR}/jni/jni_asn.c
            ${wolfcryptjni_DIR}/jni/jni_chacha.c
            ${wolfcryptjni_DIR}/jni/jni_curve25519.c
            ${wolfcryptjni_DIR}/jni/jni_des3.c
            ${wolfcryptjni_DIR}/jni/jni_dh.c
            ${wolfcryptjni_DIR}/jni/jni_ecc.c
            ${wolfcryptjni_DIR}/jni/jni_ed25519.c
            ${wolfcryptjni_DIR}/jni/jni_error.c
            ${wolfcryptjni_DIR}/jni/jni_fips.c
            ${wolfcryptjni_DIR}/jni/jni_hmac.c
            ${wolfcryptjni_DIR}/jni/jni_logging.c
            ${wolfcryptjni_DIR}/jni/jni_md5.c
            ${wolfcryptjni_DIR}/jni/jni_native_struct.c
            ${wolfcryptjni_DIR}/jni/jni_rng.c
            ${wolfcryptjni_DIR}/jni/jni_rsa.c
            ${wolfcryptjni_DIR}/jni/jni_sha.c
           )

# Include libraries needed for wolfSSL and wolfCrypt JNI libs
target_link_libraries(wolfcryptjni
                      wolfssl
                      android
                      log)
