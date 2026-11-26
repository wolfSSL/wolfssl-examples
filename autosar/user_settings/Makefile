# AUTOSAR user_settings.h build
#
# Compiles the wolfSSL sources the AUTOSAR port needs straight out of a wolfSSL
# source tree -- no configure, no CMake, no installed library. This is the shape
# an ECU build takes.
#
#   make WOLFSSL_ROOT=/path/to/wolfssl
#   make WOLFSSL_ROOT=/path/to/wolfssl check
#   make WOLFSSL_ROOT=/path/to/wolfssl CC=arm-none-eabi-gcc      (cross)
#
WOLFSSL_ROOT ?= ../../../wolfssl

CC      = gcc
CFLAGS  ?= -Os -Wall
LDFLAGS ?=
LIBS    ?=

# Kept separate from CFLAGS so that `make CFLAGS=...` on the command line adds
# to the build rather than silently dropping the include paths and
# WOLFSSL_USER_SETTINGS -- which fails in confusing ways, because the library
# sources then compile against a different configuration than the application.
# -I. is what finds user_settings.h.
ALL_CFLAGS = $(CFLAGS) -DWOLFSSL_USER_SETTINGS -I. -I$(WOLFSSL_ROOT)

# The minimal wolfCrypt set for AES-CBC, AES-CMAC and the DRBG behind
# Csm_RandomGenerate(), with this user_settings.h. Established by removing each
# file in turn and checking the link: every one below is required, and hash.c,
# hmac.c and wc_encrypt.c are not (they were in an earlier draft of this list).
# Re-check if you change the feature set.
#
# misc.c is absent on purpose -- it is #included into crypto.c unless NO_INLINE
# is defined, and compiling it standalone as well gives duplicate symbols.
WOLFCRYPT_SRC = \
	$(WOLFSSL_ROOT)/wolfcrypt/src/aes.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/cmac.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/memory.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/random.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/sha256.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/wc_port.c

# Only a DEBUG_WOLFSSL build references the logging functions; without that
# macro WOLFSSL_MSG() compiles to nothing. Kept in so that
# `make CFLAGS="-Os -Wall -DDEBUG_WOLFSSL"` links, which is how you get the
# driver to tell you why a job failed.
WOLFCRYPT_SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/logging.c

# The port itself: the Csm -> CryIf -> Crypto driver chain.
AUTOSAR_SRC = \
	$(WOLFSSL_ROOT)/wolfcrypt/src/port/autosar/csm.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/port/autosar/cryif.c \
	$(WOLFSSL_ROOT)/wolfcrypt/src/port/autosar/crypto.c

# ecu-seed.c supplies CUSTOM_RAND_GENERATE_SEED. NO_FILESYSTEM removes
# wolfSSL's own entropy source, so this is not optional.
APP_SRC = ecu-app.c ecu-seed.c

OBJ = $(notdir $(WOLFCRYPT_SRC:.c=.o)) $(notdir $(AUTOSAR_SRC:.c=.o)) \
      $(APP_SRC:.c=.o)

TARGET = ecu-app

.PHONY: all clean check size

all: $(TARGET)

# one flat object dir, so the vpath keeps the rules simple
vpath %.c $(WOLFSSL_ROOT)/wolfcrypt/src \
          $(WOLFSSL_ROOT)/wolfcrypt/src/port/autosar \
          .

# Every object depends on user_settings.h, not just on its own source. The
# README tells you to edit that file -- MAX_KEYSTORE, the AES sizes, dropping
# CMAC -- and those change struct layouts and table sizes. Without this, make
# relinks objects built under the previous configuration, which links and then
# misbehaves: exactly the trap the README's "five things that bite" warns
# about.
%.o: %.c user_settings.h
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

# $(CFLAGS) on the link line too, not just when compiling: the cross recipe in
# README.md passes -mcpu/-mthumb there, and the GCC driver needs them to pick
# the right multilib for libgcc and friends. Without them a Cortex-M build
# links against the default ARM-state libraries.
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

# What the port actually costs in flash. Useful when sizing an ECU.
size: $(TARGET)
	@size $(TARGET) 2>/dev/null || echo "size(1) not available"

check: $(TARGET)
	out=$$(./$(TARGET)) && printf '%s' "$$out" | grep -q 'ecu-app: PASS'
	@echo "PASS: user_settings checks"

clean:
	rm -f *.o $(TARGET)
