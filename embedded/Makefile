# TLS Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a -lm
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#CFLAGS+=$(QAT_FLAGS)
#LIBS+=$(QAT_LIBS)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

# build targets
SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))
LINUX_SPECIFIC=client-tls-perf \
	server-tls-epoll-perf \
	server-tls-epoll-threaded


# Intel QuickAssist
QAT_PATH=../../QAT1.6
QAT_FLAGS=-DDO_CRYPTO -DUSER_SPACE \
	-I$(QAT_PATH)/quickassist/include \
	-I$(QAT_PATH)/quickassist/include/lac \
	-I$(QAT_PATH)/quickassist/utilities/osal/include \
	-I$(QAT_PATH)/quickassist/utilities/osal/src/linux/user_space/include \
	-I$(QAT_PATH)/quickassist/lookaside/access_layer/include \
	-I$(QAT_PATH)/quickassist/lookaside/access_layer/src/common/include
QAT_LIBS=-L$(QAT_PATH) -ladf_proxy -losal -licp_qa_al_s -lpthread


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

# $(info $$OS_DET is [${OS_DET}])
# $(info $$CPU_DET is [${CPU_DET}])

.PHONY: clean all

ifneq ($(OS_DET),LINUX)
all: $(filter-out $(LINUX_SPECIFIC), $(TARGETS))
else
all: $(TARGETS)
endif

debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# add the -pthread flag to any threaded examples
%-threaded: CFLAGS+=-pthread

# compile tcp examples without the LIBS variable
%-tcp: LIBS=

# build template
%: %.c *.h
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)
