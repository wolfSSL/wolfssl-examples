TOP_PATH   := $(call my-dir)/..

WOLFSSL_DIR := wolfssl

# Build wolfSSL shared library
include $(CLEAR_VARS)
LOCAL_PATH      := $(TOP_PATH)/$(WOLFSSL_DIR)
LOCAL_MODULE    := libwolfssl
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := -DOPENSSL_EXTRA -DWOLFSSL_DTLS -D_POSIX_THREADS -DNDEBUG \
				-DPERSIST_SESSION_CACHE -DPERSIST_CERT_CACHE -DATOMIC_USER \
				-DHAVE_PK_CALLBACKS -DNO_DSA -DHAVE_ECC -DTFM_ECC256 \
				-DECC_SHAMIR -DNO_MD4 -DNO_HC128 -DNO_RABBIT \
				-DHAVE_OCSP -DHAVE_CRL -DWOLFSSL_JNI -DHAVE_DH \
				-DUSE_FAST_MATH -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT \
				-DWC_RSA_BLINDING -DTFM_NO_ASM \
				-Wall
LOCAL_SRC_FILES := src/crl.c \
				   src/internal.c \
				   src/io.c \
				   src/keys.c \
				   src/ocsp.c \
				   src/sniffer.c \
				   src/ssl.c \
				   src/tls.c \
				   wolfcrypt/src/aes.c \
				   wolfcrypt/src/arc4.c \
				   wolfcrypt/src/asm.c \
				   wolfcrypt/src/asn.c \
				   wolfcrypt/src/blake2b.c \
				   wolfcrypt/src/camellia.c \
				   wolfcrypt/src/chacha.c \
				   wolfcrypt/src/chacha20_poly1305.c \
				   wolfcrypt/src/cmac.c \
				   wolfcrypt/src/coding.c \
				   wolfcrypt/src/compress.c \
				   wolfcrypt/src/curve25519.c \
				   wolfcrypt/src/des3.c \
				   wolfcrypt/src/dh.c \
				   wolfcrypt/src/dsa.c \
				   wolfcrypt/src/ecc.c \
				   wolfcrypt/src/ecc_fp.c \
				   wolfcrypt/src/ed25519.c \
				   wolfcrypt/src/error.c \
				   wolfcrypt/src/fe_low_mem.c \
				   wolfcrypt/src/fe_operations.c \
				   wolfcrypt/src/ge_low_mem.c \
				   wolfcrypt/src/ge_operations.c \
				   wolfcrypt/src/hash.c \
				   wolfcrypt/src/hmac.c \
				   wolfcrypt/src/integer.c \
				   wolfcrypt/src/logging.c \
				   wolfcrypt/src/md2.c \
				   wolfcrypt/src/md4.c \
				   wolfcrypt/src/md5.c \
				   wolfcrypt/src/memory.c \
				   wolfcrypt/src/pkcs12.c \
				   wolfcrypt/src/pkcs7.c \
				   wolfcrypt/src/poly1305.c \
				   wolfcrypt/src/pwdbased.c \
				   wolfcrypt/src/random.c \
				   wolfcrypt/src/ripemd.c \
				   wolfcrypt/src/rsa.c \
				   wolfcrypt/src/sha.c \
				   wolfcrypt/src/sha256.c \
				   wolfcrypt/src/sha512.c \
				   wolfcrypt/src/signature.c \
				   wolfcrypt/src/srp.c \
				   wolfcrypt/src/tfm.c \
				   wolfcrypt/src/wc_encrypt.c \
				   wolfcrypt/src/wc_port.c \
				   wolfcrypt/src/wolfevent.c \
				   wolfcrypt/src/wolfmath.c
include $(BUILD_SHARED_LIBRARY)


## Build wolfSSL JNI library
include $(CLEAR_VARS)
LOCAL_PATH       := $(TOP_PATH)/wolfssljni
LOCAL_MODULE     := libwolfssljni
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/native
LOCAL_C_INCLUDES := $(LOCAL_PATH)/native
LOCAL_SRC_FILES  := native/com_wolfssl_WolfSSL.c \
				    native/com_wolfssl_WolfSSLContext.c \
				    native/com_wolfssl_WolfSSLSession.c \
				    native/com_wolfssl_wolfcrypt_ECC.c \
			  	    native/com_wolfssl_wolfcrypt_RSA.c
LOCAL_CFLAGS := -DOPENSSL_EXTRA -DWOLFSSL_DTLS -D_POSIX_THREADS -DNDEBUG \
				-DPERSIST_SESSION_CACHE -DPERSIST_CERT_CACHE -DATOMIC_USER \
				-DHAVE_PK_CALLBACKS -DNO_DSA -DHAVE_ECC -DTFM_ECC256 \
				-DECC_SHAMIR -DNO_MD4 -DNO_HC128 -DNO_RABBIT \
				-DHAVE_OCSP -DHAVE_CRL -DWOLFSSL_JNI -DHAVE_DH \
				-DUSE_FAST_MATH -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT \
				-DWC_RSA_BLINDING -DTFM_NO_ASM \
				-Wall
LOCAL_SHARED_LIBRARIES := libwolfssl
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PATH   := $(TOP_PATH)/jni
LOCAL_MODULE := libjnitest
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SRC_FILES := jnitest.c
LOCAL_CFLAGS := -DOPENSSL_EXTRA -DWOLFSSL_DTLS -D_POSIX_THREADS -DNDEBUG \
				-DPERSIST_SESSION_CACHE -DPERSIST_CERT_CACHE -DATOMIC_USER \
				-DHAVE_PK_CALLBACKS -DNO_DSA -DHAVE_ECC -DTFM_ECC256 \
				-DECC_SHAMIR -DNO_MD4 -DNO_HC128 -DNO_RABBIT \
				-DHAVE_OCSP -DHAVE_CRL -DWOLFSSL_JNI -DHAVE_DH \
				-DUSE_FAST_MATH -DTFM_TIMING_RESISTANT -DECC_TIMING_RESISTANT \
				-DWC_RSA_BLINDING -DTFM_NO_ASM \
				-Wall

LOCAL_SHARED_LIBRARIES := libwolfssl
include $(BUILD_SHARED_LIBRARY)

