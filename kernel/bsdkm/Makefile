# example module name and sources
KMOD=bsd_example
SRCS=bsd_example.c

# path to wolfssl dir
WOLFSSL_DIR=../../../wolfssl/

# suppress wolfcrypt/src/misc.c drops const qualifier
CFLAGS+= -Wno-cast-qual
CFLAGS+= -Wno-error=cast-qual
CFLAGS+= -I/usr/include
CFLAGS+=-I${WOLFSSL_DIR} -DWOLFSSL_USE_OPTIONS_H -DWOLFSSL_CUSTOM_CONFIG

# point to live kernel kmod dot mk
.include "/usr/src/sys/conf/kmod.mk"
