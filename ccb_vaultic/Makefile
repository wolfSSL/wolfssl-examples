# Pick up cross-compiler info from environment
CC=${NDK_CC}
AS=${NDK_AS} 
RANLIB=${NDK_RANLIB} 
AR=${NDK_AR} 
LD=${NDK_LD}

# Relative path to wolfSSL source
WOLFSSL_DIR?=../../bill_wolfssl

# Relative path to VaultIC dev kit source
VAULTIC_DIR?=VaultIC-TLS_420/vaultic_tls-4xx

# Common settings and files
CFLAGS+=-I. -O2
OBJS=ccb_vaultic.o

# Uncomment to enable verbose debug logging
CFLAGS+=-DCCBVAULTIC_DEBUG
CFLAGS+=-DCCBVAULTIC_DEBUG_TIMING
#CFLAGS+=-DCCBVAULTIC_DEBUG_ALL
#CFLAGS+=-DSPI_TRACE_ERRORS

# Optionally disable certain kinds of offload
#CFLAGS+=-DNO_CCBVIC_AES
CFLAGS+=-DNO_CCBVIC_SHA
#CFLAGS+=-DNO_CCBVIC_RSA

# wolfSSL settings and files
# ./configure --enable-cryptocb --enable-static --disable-shared --host aarch64-linux-android
CFLAGS+=-I$(WOLFSSL_DIR)
LDFLAGS+=-L$(WOLFSSL_DIR)/src/.libs -lwolfssl -lm

# VaultIC settings and files.  These must match the vault_tls_config.h
VAULTIC_CHIP=420
CHIP_TARGET=TARGETCHIP_VAULTIC_$(VAULTIC_CHIP)
CFLAGS+= -D$(CHIP_TARGET)
CFLAGS+= -DUSE_SPI

VAULTIC_ELIB?=$(VAULTIC_DIR)/VaultIC-Elib_$(VAULTIC_CHIP)/src
CFLAGS+=-I$(VAULTIC_ELIB)/common
CFLAGS+=-I$(VAULTIC_ELIB)/device/vaultic_4XX_family 

VAULTIC_TLS?=$(VAULTIC_DIR)/lib/VaultIC_420_TLS_Lib/target/raspberry/dist
CFLAGS+=-I$(VAULTIC_TLS)
#LDFLAGS+=-L$(VAULTIC_TLS)/VaultIC_420_TLS_Lib-SPI-Debug -lvaultic420_tls
LDFLAGS+=-L$(VAULTIC_TLS)/VaultIC_420_TLS_Lib-SPI-Release -lvaultic420_tls

#Test and Benchmark targets
CFLAGS+=-DWC_USE_DEVID=0x56490420 -DBENCH_EMBEDDED -DNO_MAIN_DRIVER
TEST_OBJS:=$(WOLFSSL_DIR)/wolfcrypt/test/test.o main-test.o
BENCH_OBJS:=$(WOLFSSL_DIR)/wolfcrypt/benchmark/benchmark.o main-bench.o


all: wolfcrypt-test wolfcrypt-benchmark

wolfcrypt-test: $(OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

wolfcrypt-benchmark: $(OBJS) $(BENCH_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

lib: $(OBJS)
	cp $(VAULTIC_TLS)/VaultIC_420_TLS_Lib-SPI-Release/libvaultic420_tls.a libccbvaultic.a
	$(AR) -r libccbvaultic.a $(OBJS)
	
clean:
	rm -f *.o *.d wolfcrypt-test wolfcrypt-benchmark
	rm -f $(WOLFSSL_DIR)/wolfcrypt/benchmark/*.o
	rm -f $(WOLFSSL_DIR)/wolfcrypt/test/*.o
