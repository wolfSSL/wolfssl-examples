# Makefile for the wolfCrypt test and benchmark on Infineon PSOC Edge E8
#
# Target: KIT_PSE84_AI (PSE846GPS2DBZC4A), CM33 Secure, run in place from QSPI
# Usage:  make                 - build wolfcrypt_test and wolfcrypt_bench
#         make PSOC_CRYPTO=0   - software only, for comparison
#         make PSOC_CRYPTO_ECC=1 - ECDSA verify on the engine as well
#         make clean
#
# Copyright (C) 2006-2026 wolfSSL Inc.

TOOLCHAIN ?= arm-none-eabi-
CC = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE = $(TOOLCHAIN)size

WOLFSSL_ROOT ?= ../../../wolfssl

# Infineon device support, from github.com/Infineon (see README.md).
PSE84_DSL ?= ../../../mtb-dsl-pse8xxgp
PSE84_TEMPLATE ?= ../../../mtb-template-pse8xxgp
CORE_LIB ?= ../../../core-lib
CMSIS ?= ../../../CMSIS_6
PSE84_PART ?= pse846gps2dbzc4a

PSOC_CRYPTO ?= 1
PSOC_CRYPTO_ECC ?= 0

BUILD_DIR ?= ./Build$(if $(filter 0,$(PSOC_CRYPTO)),-sw)$(if $(filter 1,$(PSOC_CRYPTO_ECC)),-ecc)

ARCHFLAGS = -mcpu=cortex-m33 -mthumb -mabi=aapcs

CFLAGS = $(ARCHFLAGS) -std=gnu99 -Wall -O2 -g
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -DWOLFSSL_USER_SETTINGS -DWOLFSSL_IGNORE_FILE_WARN
CFLAGS += -DCOMPONENT_CM33 -DCORE_NAME_CM33_0 -DCOMPONENT_SECURE_DEVICE
CFLAGS += -D$(shell echo $(PSE84_PART) | tr a-z A-Z)
CFLAGS += -DPSE84_PART_HEADER='"$(PSE84_PART)_s.h"'
CFLAGS += -I. -I$(WOLFSSL_ROOT)
CFLAGS += -I$(PSE84_DSL)/pdl/drivers/include
CFLAGS += -I$(PSE84_DSL)/pdl/drivers/third_party/ethernet/include
CFLAGS += -I$(PSE84_DSL)/pdl/devices/include
CFLAGS += -I$(PSE84_DSL)/pdl/devices/include/ip
CFLAGS += -I$(PSE84_TEMPLATE)/files
CFLAGS += -I$(PSE84_TEMPLATE)/files/devices/include
CFLAGS += -I$(CORE_LIB)/include
CFLAGS += -I$(CMSIS)/CMSIS/Core/Include
ifeq ($(PSOC_CRYPTO),1)
CFLAGS += -DWOLFSSL_PSOC6_CRYPTO
endif
ifneq ($(PSOC_CRYPTO_ECC),1)
CFLAGS += -DNO_PSOC6_CRYPTO_ECC
endif

LDFLAGS = $(ARCHFLAGS) --specs=nano.specs -nostartfiles
LDFLAGS += -Wl,--gc-sections -T./linker.ld
LIBS = -lm

WOLF = $(WOLFSSL_ROOT)/wolfcrypt/src
SRC_WOLF = $(WOLF)/aes.c $(WOLF)/ecc.c $(WOLF)/error.c $(WOLF)/hash.c \
    $(WOLF)/hmac.c $(WOLF)/logging.c $(WOLF)/memory.c $(WOLF)/misc.c \
    $(WOLF)/random.c $(WOLF)/sha.c $(WOLF)/sha256.c $(WOLF)/sha512.c \
    $(WOLF)/sha3.c $(WOLF)/sp_int.c $(WOLF)/sp_cortexm.c $(WOLF)/wc_port.c \
    $(WOLF)/wolfmath.c $(WOLF)/asn.c $(WOLF)/coding.c $(WOLF)/kdf.c \
    $(WOLF)/wc_encrypt.c $(WOLF)/signature.c \
    $(WOLF)/port/cypress/psoc6_crypto.c

# PDL crypto core drivers: the engine, its TRNG, and the ECC drivers when
# ECDSA verify runs on the engine.
PDL = $(PSE84_DSL)/pdl/drivers/source
SRC_PDL = $(PDL)/cy_crypto_core_hw.c $(PDL)/cy_crypto_core_mem_v2.c \
    $(PDL)/cy_crypto_core_sha_v2.c $(PDL)/cy_crypto_core_aes_v2.c \
    $(PDL)/cy_crypto_core_vu.c $(PDL)/cy_crypto_core_trng.c
ifeq ($(PSOC_CRYPTO_ECC),1)
SRC_PDL += $(PDL)/cy_crypto_core_ecc_domain_params.c \
    $(PDL)/cy_crypto_core_ecc_nist_p.c $(PDL)/cy_crypto_core_ecc_ecdsa.c \
    $(PDL)/cy_crypto_core_ecc_key_gen.c $(PDL)/cy_crypto_core_ecc_ec25519.c
endif

SRC_TEST = $(WOLFSSL_ROOT)/wolfcrypt/test/test.c
SRC_BENCH = $(WOLFSSL_ROOT)/wolfcrypt/benchmark/benchmark.c

objs = $(addprefix $(BUILD_DIR)/$(1)/, $(notdir $(2:.c=.o)))
OBJS_COMMON = $(call objs,common,$(SRC_WOLF) $(SRC_PDL) startup.c board.c)
OBJS_TEST = $(OBJS_COMMON) $(call objs,test,main.c $(SRC_TEST))
OBJS_BENCH = $(OBJS_COMMON) $(call objs,bench,main.c $(SRC_BENCH))

vpath %.c . $(sort $(dir $(SRC_WOLF) $(SRC_PDL) $(SRC_TEST) $(SRC_BENCH)))

.PHONY: all clean check-deps

all: check-deps $(BUILD_DIR)/wolfcrypt_test.bin $(BUILD_DIR)/wolfcrypt_bench.bin
	$(SIZE) $(BUILD_DIR)/wolfcrypt_test.elf $(BUILD_DIR)/wolfcrypt_bench.elf

check-deps:
	@for d in $(WOLFSSL_ROOT)/wolfcrypt $(PSE84_DSL)/pdl \
	    $(PSE84_TEMPLATE)/files $(CORE_LIB)/include $(CMSIS)/CMSIS; do \
	    test -d $$d || { echo "missing $$d (see README.md)"; exit 1; }; done

$(BUILD_DIR)/common/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/test/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/bench/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DBENCH_APP -c -o $@ $<

$(BUILD_DIR)/wolfcrypt_test.elf: $(OBJS_TEST)
	$(CC) $(LDFLAGS) -Wl,-Map=$(@:.elf=.map) -o $@ $^ $(LIBS)

$(BUILD_DIR)/wolfcrypt_bench.elf: $(OBJS_BENCH)
	$(CC) $(LDFLAGS) -Wl,-Map=$(@:.elf=.map) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf ./Build ./Build-sw ./Build-ecc ./Build-sw-ecc
