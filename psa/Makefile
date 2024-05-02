CC       = gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS   ?= -Wall -I$(INCLUDE_PATH)
LIBS     ?= -L$(WOLFSSL_INSTALL_DIR)/lib -lm

# PSA implementation
PSA_INCLUDE ?=
PSA_WOLFSSL_INSTALL_DIR ?=

# option variables
DYN_LIB         = -lwolfssl -lm
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

CFLAGS += -I$(PSA_INCLUDE)
LIBS += $(PSA_WOLFSSL_INSTALL_DIR)

TARGETS=client-tls13-ecc-psa server-tls13-ecc-psa

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)
clean:
	@rm -f $(TARGETS) *.o
