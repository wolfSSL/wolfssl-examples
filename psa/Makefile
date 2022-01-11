CC       = gcc
LIB_PATH = /usr/local
CFLAGS   ?= -Wall -I$(INCLUDE_PATH)
LIBS     ?= -L$(LIB_PATH)/lib -lm

# PSA implementation
PSA_INCLUDE ?=
PSA_LIB_PATH ?=

# option variables
DYN_LIB         = -lwolfssl -lm
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

CFLAGS += -I$(PSA_INCLUDE)
LIBS += $(PSA_LIB_PATH)

TARGETS=client-tls13-ecc-psa server-tls13-ecc-psa

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)
clean:
	@rm -f $(TARGETS) *.o
