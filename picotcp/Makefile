# PicoTCP Example Makefile

CC           = gcc
LIB_PATH     = /usr/local
PICOTCP_PATH = ../../picotcp
CFLAGS       = -Wall -I$(LIB_PATH)/include -I$(PICOTCP_PATH)/build/include
LIBS         = -L$(LIB_PATH)/lib -lm -pthread

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(PICOTCP_PATH)/build/lib/libpicotcp.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
SILENCE_FLAGS   = -Wno-address-of-packed-member -Wno-discarded-qualifiers
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
CFLAGS+=$(SILENCE_FLAGS)
LIBS+=$(STATIC_LIB) $(DYN_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

# OS / CPU Detection
OS_DET=UNKNOWN
CPU_DET=UNKNOWN
ifeq ($(OS),Windows_NT)
    OS_DET=WIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CPU_DET=AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CPU_DET=AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CPU_DET=IA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS_DET=LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        OS_DET=OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CPU_DET=AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CPU_DET=IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CPU_DET=ARM
    endif
endif

.PHONY: clean all

all: $(TARGETS)

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
