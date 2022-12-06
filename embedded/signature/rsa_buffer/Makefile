WOLFROOT = ../../../../wolfssl

CFLAGS =-DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT) -Os
ASFLAGS=-DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT)

OBJ = \
	$(WOLFROOT)/wolfcrypt/src/rsa.o\
	$(WOLFROOT)/wolfcrypt/src/sha256.o\
	$(WOLFROOT)/wolfcrypt/src/hash.o\
	$(WOLFROOT)/wolfcrypt/src/random.o\
	$(WOLFROOT)/wolfcrypt/src/asn.o\
	$(WOLFROOT)/wolfcrypt/src/wc_port.o\
	$(WOLFROOT)/wolfcrypt/src/coding.o\
	$(WOLFROOT)/wolfcrypt/src/memory.o\

OBJ_SP_C32 := \
	$(WOLFROOT)/wolfcrypt/src/sp_int.o\
	$(WOLFROOT)/wolfcrypt/src/sp_c32.o\

OBJ_SP_ARM64 := \
	$(WOLFROOT)/wolfcrypt/src/sp_int.o\
	$(WOLFROOT)/wolfcrypt/src/sp_arm64.o\


OBJ_SP_X86_64 := \
	$(WOLFROOT)/wolfcrypt/src/sp_int.o\
	$(WOLFROOT)/wolfcrypt/src/cpuid.o\
	$(WOLFROOT)/wolfcrypt/src/sp_x86_64.o\
	$(WOLFROOT)/wolfcrypt/src/sp_x86_64_asm.o\

OBJ_TFM := \
	$(WOLFROOT)/wolfcrypt/src/wolfmath.o\
	$(WOLFROOT)/wolfcrypt/src/tfm.o\


.PHONY: all clean mem size 

ifeq ($(math) $(arch),sp x64) 
ASFLAGS+= -DSP_X86_64_FLAG
OBJ += $(OBJ_SP_X86_64)
else ifeq ($(math) $(arch),sp arm64)
CFLAGS += -DSP_ARM64_FLAG
OBJ += $(OBJ_SP_ARM64)
else ifeq ($(math), tfm)
CFLAGS += -DTFM_FLAG
OBJ += $(OBJ_TFM)
else 
CFLAGS += -DSP_FLAG
OBJ += $(OBJ_SP_C32)
endif

all: verify sign bench

mem:CFLAGS+= -DDEBUG_MEMORY 
mem: verify sign

verify: $(OBJ)
	$(CC) $(CFLAGS) -o verify verify.c $(OBJ)
sign: $(OBJ)
	$(CC) $(CFLAGS) -o sign sign.c $(OBJ)
bench: $(OBJ)
	$(CC) $(CFLAGS) -DBENCHMARK -o bench verify.c $(OBJ)
clean:
	rm -f verify sign bench $(WOLFROOT)/wolfcrypt/src/*.o
size :
	size $(OBJ)
