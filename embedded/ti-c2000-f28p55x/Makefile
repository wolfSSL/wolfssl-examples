# Makefile - headless wolfCrypt test/benchmark build for TI LAUNCHXL-F28P55X
#            (TMS320F28P550SJ, C2000 C28x DSP), built from flash with cl2000.
#
# Copyright (C) 2006-2026 wolfSSL Inc.  GPLv3 - see project headers.
#
# Usage:
#   make CGT_ROOT=/path/to/ti-cgt-c2000_xx.y.z [C2000WARE=/path/to/C2000Ware]
#
# CGT_ROOT must point at a TI C2000 codegen install (the dir containing
# bin/cl2000).  It ships with Code Composer Studio or as a standalone package.
# C2000WARE defaults to ~/ti/C2000Ware_26_01_00_00.
#
# This is the SCOPED bring-up build: SHA-2, SHA-3/SHAKE, and ML-DSA-87 verify
# only.  See README.md.

CGT_ROOT  ?=
C2000WARE ?= $(HOME)/ti/C2000Ware_26_01_00_00

# wolfSSL source tree (this example lives in wolfssl-examples; point WOLFROOT
# at a wolfSSL checkout that has the CHAR_BIT!=8 fixes).  Override on the
# command line, e.g. make WOLFROOT=/path/to/wolfssl.
WOLFROOT  ?= $(abspath $(CURDIR)/../../../wolfssl-alt)

DEV       := $(C2000WARE)/device_support/f28p55x
DRV       := $(C2000WARE)/driverlib/f28p55x/driverlib
DRVLIB    := $(DRV)/ccs/Release

OUT       := out
TARGET    := $(OUT)/wolfcrypt_f28p55x.out
MAP       := $(OUT)/wolfcrypt_f28p55x.map

ifeq ($(CGT_ROOT),)
$(error Set CGT_ROOT to your TI C2000 codegen install, e.g. \
  make CGT_ROOT=$(HOME)/ti/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS)
endif

CL := $(CGT_ROOT)/bin/cl2000

INCS := \
  -I$(CGT_ROOT)/include \
  -I$(DRV) \
  -I$(DEV)/common/include \
  -I$(DEV)/headers/include \
  -I$(WOLFROOT) \
  -I$(CURDIR)/Header

# --float_support=fpu32 and --abi=eabi must match the prebuilt driverlib.lib.
# Define WOLF_C2000_SCI_STDOUT to route printf to SCIA (XDS110 COM); omit it to
# use the CCS console (CIO over JTAG) for first light.
CFLAGS := \
  -v28 --float_support=fpu32 --tmu_support=tmu1 --abi=eabi \
  -O2 \
  --define=_LAUNCHXL_F28P55X \
  --define=_FLASH \
  --define=WOLFSSL_USER_SETTINGS \
  --gen_func_subsections=on \
  --diag_warning=225 --display_error_number \
  $(EXTRA_CFLAGS) \
  $(INCS)

# Output routing: SCI/UART is the DEFAULT -- printf streams to SCIA on the XDS110
# virtual COM (GPIO28/29) at 115200 8N1, so uart-monitor can tail it with NO
# debugger attached (an active JTAG session blocks the backchannel).  Build with
# CIO=1 to use the CCS/JTAG console (CIO) instead -- required by the DSS JTAG
# capture scripts (run_capture.js / regress.sh), which read printf over JTAG.
CIO ?= 0
ifeq ($(CIO),0)
  CFLAGS += --define=WOLF_C2000_SCI_STDOUT
endif

LFLAGS := \
  --reread_libs --warn_sections \
  -i$(CGT_ROOT)/lib -i$(DRVLIB) \
  -m $(MAP)

# wolfCrypt source subset (the only files that compile under CHAR_BIT==16).
WC_SRCS := \
  $(WOLFROOT)/wolfcrypt/src/error.c \
  $(WOLFROOT)/wolfcrypt/src/wc_port.c \
  $(WOLFROOT)/wolfcrypt/src/memory.c \
  $(WOLFROOT)/wolfcrypt/src/logging.c \
  $(WOLFROOT)/wolfcrypt/src/misc.c \
  $(WOLFROOT)/wolfcrypt/src/coding.c \
  $(WOLFROOT)/wolfcrypt/src/hash.c \
  $(WOLFROOT)/wolfcrypt/src/sha256.c \
  $(WOLFROOT)/wolfcrypt/src/sha512.c \
  $(WOLFROOT)/wolfcrypt/src/sha3.c \
  $(WOLFROOT)/wolfcrypt/src/wc_mldsa.c \
  $(WOLFROOT)/wolfcrypt/src/random.c

# ECC=1 adds ECDSA/ECDH over SECP256R1 via SP single-precision math.
ECC ?= 0
ifeq ($(ECC),1)
  CFLAGS  += --define=WOLF_ECC
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/ecc.c \
    $(WOLFROOT)/wolfcrypt/src/wolfmath.c \
    $(WOLFROOT)/wolfcrypt/src/sp_int.c \
    $(WOLFROOT)/wolfcrypt/src/sp_c32.c
endif

# MLKEM=1 adds ML-KEM-768 (FIPS 203) via the wolfCrypt C implementation.
MLKEM ?= 0
ifeq ($(MLKEM),1)
  CFLAGS  += --define=WOLF_MLKEM
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/wc_mlkem.c \
    $(WOLFROOT)/wolfcrypt/src/wc_mlkem_poly.c
endif

# AES=1 adds AES-CBC/CTR/CFB/GCM (software, table-driven; GCM_SMALL GHASH).
AES ?= 0
ifeq ($(AES),1)
  CFLAGS  += --define=WOLF_AES
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/aes.c \
    $(WOLFROOT)/wolfcrypt/src/wc_encrypt.c
endif

# X25519=1 adds Curve25519 (X25519) + Ed25519 (default fe[10] 32-bit backend).
X25519 ?= 0
ifeq ($(X25519),1)
  CFLAGS  += --define=WOLF_25519
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/curve25519.c \
    $(WOLFROOT)/wolfcrypt/src/ed25519.c \
    $(WOLFROOT)/wolfcrypt/src/fe_operations.c \
    $(WOLFROOT)/wolfcrypt/src/ge_operations.c
endif

# X448=1 adds Curve448 (X448) + Ed448 (CURVE448_SMALL/ED448_SMALL byte backend).
X448 ?= 0
ifeq ($(X448),1)
  CFLAGS  += --define=WOLF_448
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/curve448.c \
    $(WOLFROOT)/wolfcrypt/src/ed448.c \
    $(WOLFROOT)/wolfcrypt/src/fe_448.c \
    $(WOLFROOT)/wolfcrypt/src/ge_448.c
endif

# HKDF=1 adds HMAC + HKDF (RFC 2104 / RFC 5869) on top of the SHA-2/3 hashes.
HKDF ?= 0
ifeq ($(HKDF),1)
  CFLAGS  += --define=WOLF_HKDF
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/hmac.c \
    $(WOLFROOT)/wolfcrypt/src/kdf.c
endif

# CHACHA=1 adds ChaCha20-Poly1305 AEAD (RFC 8439).
CHACHA ?= 0
ifeq ($(CHACHA),1)
  CFLAGS  += --define=WOLF_CHACHA
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/chacha.c \
    $(WOLFROOT)/wolfcrypt/src/poly1305.c \
    $(WOLFROOT)/wolfcrypt/src/chacha20_poly1305.c
endif

# AESEXTRA=1 adds AES-CCM, CMAC and GMAC on top of the AES core.
AESEXTRA ?= 0
ifeq ($(AESEXTRA),1)
  CFLAGS  += --define=WOLF_AES --define=WOLF_AESEXTRA
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/aes.c \
    $(WOLFROOT)/wolfcrypt/src/cmac.c \
    $(WOLFROOT)/wolfcrypt/src/wc_encrypt.c
endif

# RSA=1 adds RSA verify (SP math backend, shared with the ECC P-256 build).
RSA ?= 0
ifeq ($(RSA),1)
  CFLAGS  += --define=WOLF_RSA
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/rsa.c \
    $(WOLFROOT)/wolfcrypt/src/asn.c \
    $(WOLFROOT)/wolfcrypt/src/wolfmath.c \
    $(WOLFROOT)/wolfcrypt/src/sp_int.c \
    $(WOLFROOT)/wolfcrypt/src/sp_c32.c
endif

# SHA1=1 adds (legacy) SHA-1.
SHA1 ?= 0
ifeq ($(SHA1),1)
  CFLAGS  += --define=WOLF_SHA1
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/sha.c
endif

# RSASIGN=1 adds RSA-2048 sign (private key, SP CRT path; raw key, no ASN).
RSASIGN ?= 0
ifeq ($(RSASIGN),1)
  # KAT-only: the upstream rsa test/benchmark need ASN (wc_RsaPrivateKeyDecode),
  # which this raw-key, ASN-free build doesn't compile.
  CFLAGS  += --define=WOLF_RSASIGN --define=NO_CRYPT_TEST \
             --define=NO_CRYPT_BENCHMARK
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/rsa.c \
    $(WOLFROOT)/wolfcrypt/src/wolfmath.c \
    $(WOLFROOT)/wolfcrypt/src/sp_int.c \
    $(WOLFROOT)/wolfcrypt/src/sp_c32.c
endif

# DH=1 adds Diffie-Hellman (FFDHE-2048) via the SP math backend.
DH ?= 0
ifeq ($(DH),1)
  CFLAGS  += --define=WOLF_DH
  WC_SRCS += \
    $(WOLFROOT)/wolfcrypt/src/dh.c \
    $(WOLFROOT)/wolfcrypt/src/wolfmath.c \
    $(WOLFROOT)/wolfcrypt/src/sp_int.c \
    $(WOLFROOT)/wolfcrypt/src/sp_c32.c
endif

HARNESS_SRCS := \
  $(WOLFROOT)/wolfcrypt/test/test.c \
  $(WOLFROOT)/wolfcrypt/benchmark/benchmark.c

BSP_SRCS := \
  $(CURDIR)/Source/wolf_main.c \
  $(DEV)/common/source/device.c

ASM_SRCS := \
  $(DEV)/common/source/f28p55x_codestartbranch.asm

# SIGN=1 builds the full ML-DSA-87 (keygen + sign + verify) demo.  It needs a
# 32 KW heap, so the wolfcrypt test/benchmark harness is left out and a
# dedicated linker script is used.  Default (SIGN=0) is the verify-only build
# with the full test/benchmark harness.
SIGN ?= 0
# BENCH=1 runs ONLY the benchmark (skips wolfcrypt_test).  wolfcrypt_test's
# heap high-water leaves too little for the benchmark's block buffers on this
# RAM-limited part, so run them in separate images: default = KATs + test, and
# BENCH=1 = KATs + benchmark.
BENCH ?= 0
ifeq ($(SIGN),1)
  CFLAGS  += --define=WOLF_MLDSA_SIGN --define=NO_CRYPT_TEST \
             --define=NO_CRYPT_BENCHMARK
  LNKCMD  := $(CURDIR)/28p55x_wolf_sign_lnk.cmd
  HARNESS_SRCS :=
else
  LNKCMD  := $(CURDIR)/28p55x_wolf_flash_lnk.cmd
  ifeq ($(BENCH),1)
    CFLAGS += --define=NO_CRYPT_TEST
  endif
endif

# MEMPROF=1 installs a heap high-water tracker (via wolfSSL_SetAllocators) and
# the stack paint, and prints per-op "MEMPROF ..." RAM lines.  Measurement-only
# (uses USE_WOLFSSL_MEMORY instead of NO_WOLFSSL_MEMORY).  Combine with SIGN=1
# or MLKEM=1 (or default for ML-DSA verify).
MEMPROF ?= 0
ifeq ($(MEMPROF),1)
  CFLAGS += --define=WOLF_MEM_PROFILE
endif

ALL_SRCS := $(WC_SRCS) $(HARNESS_SRCS) $(BSP_SRCS) $(ASM_SRCS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(ALL_SRCS) $(LNKCMD) user_settings_dep
	@mkdir -p $(OUT)
	$(CL) $(CFLAGS) \
	  $(ALL_SRCS) \
	  -z $(LFLAGS) \
	  --output_file=$(TARGET) \
	  $(LNKCMD) \
	  -l driverlib.lib -l libc.a

# touch-only dependency so editing user_settings.h forces a rebuild
user_settings_dep: Header/user_settings.h

# Compile-only guard (no link, no C2000Ware): mirrors the upstream CI job at
# wolfssl/.github/workflows/ti-c2000-compile.yml.  Builds the CHAR_BIT!=8
# wolfCrypt subset with cl2000 to catch regressions locally.
.PHONY: compile-ci
compile-ci:
	CGT_ROOT=$(CGT_ROOT) $(WOLFROOT)/IDE/C2000/compile.sh

clean:
	rm -rf $(OUT) *.obj *.map *.asm
