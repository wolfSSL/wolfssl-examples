# Common settings and files
CFLAGS+=-I. 
OBJS=wisekey_vaultic.o

# Uncomment to enable verbose debug logging
#CFLAGS+=-DDEBUG_VAULTIC
#CFLAGS+=-DSPI_TRACE_ERRORS

# WolfSSL settings and files
# ./configure --enable-cryptocb --enable-static --disable-shared --host aarch64-linux-android
WOLFSSL?=../../bill_wolfssl
CFLAGS+=-I$(WOLFSSL)
CFLAGS+=-DWOLFSSL_USER_SETTINGS
LDFLAGS+=-L$(WOLFSSL)/src/.libs -lwolfssl -lm

# VaultIC settings and files
VAULTIC_CHIP=420
CHIP_TARGET=TARGETCHIP_VAULTIC_$(VAULTIC_CHIP)
CFLAGS+= -D$(CHIP_TARGET)
CFLAGS+= -DUSE_SPI

VAULTIC?=VaultIC-TLS_420/vaultic_tls-4xx
VAULTIC_ELIB?=$(VAULTIC)/VaultIC-Elib_$(VAULTIC_CHIP)/src
CFLAGS+=-I$(VAULTIC_ELIB)/common
CFLAGS+=-I$(VAULTIC_ELIB)/device/vaultic_4XX_family 

VAULTIC_TLS?=$(VAULTIC)/lib/VaultIC_420_TLS_Lib/target/raspberry/dist
CFLAGS+=-I$(VAULTIC_TLS)
#LDFLAGS+=-L$(VAULTIC_TLS)/VaultIC_420_TLS_Lib-SPI-Debug -lvaultic420_tls
LDFLAGS+=-L$(VAULTIC_TLS)/VaultIC_420_TLS_Lib-SPI-Release -lvaultic420_tls

#Test and Benchmark targets
TEST_OBJS:=$(WOLFSSL)/wolfcrypt/test/test.o main-test.o
BENCH_OBJS:=$(WOLFSSL)/wolfcrypt/benchmark/benchmark.o main-bench.o


all: wolfcrypt-test wolfcrypt-benchmark

wolfcrypt-test: $(OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

wolfcrypt-benchmark: $(OBJS) $(BENCH_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o *.d wolfcrypt-test wolfcrypt-benchmark
	rm -f $(WOLFSSL)/wolfcrypt/benchmark/*.o
	rm -f $(WOLFSSL)/wolfcrypt/test/*.o
