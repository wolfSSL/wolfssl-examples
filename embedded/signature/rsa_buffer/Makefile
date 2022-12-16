WOLFROOT = ../../../../wolfssl

CFLAGS = $(EX_CFLAGS) -DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT) -Os
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

OBJ_SP_C64 := \
	$(WOLFROOT)/wolfcrypt/src/sp_int.o\
	$(WOLFROOT)/wolfcrypt/src/sp_c64.o\

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


.PHONY: all clean size mem bench

ifeq ($(math) $(arch),sp x64) 
ASFLAGS+= -DSP_X86_64_FLAG
OBJ += $(OBJ_SP_X86_64)
else ifeq ($(math) $(arch),sp arm64)
CFLAGS += -DSP_ARM64_FLAG
OBJ += $(OBJ_SP_ARM64)
else ifeq ($(math) $(arch),sp c64)
CFLAGS += -DSP_C64_FLAG
OBJ += $(OBJ_SP_C64)
else ifeq ($(math) $(arch),sp c32)
CFLAGS += -DSP_C32_FLAG
OBJ += $(OBJ_SP_C32)
else ifeq ($(math), tfm)
CFLAGS += -DTFM_FLAG
OBJ += $(OBJ_TFM)
else 
CFLAGS += -DSP_C64_FLAG
OBJ += $(OBJ_SP_C64)
endif

all: verify sign bench mem

verify: $(OBJ)
	$(CC) $(CFLAGS) -o verify verify.c $(OBJ)
sign: $(OBJ)
	$(CC) $(CFLAGS) -o sign sign.c $(OBJ)
bench: $(OBJ)
	$(CC) $(CFLAGS) -DBENCHMARK -o verify_bench verify.c $(OBJ)
mem: $(OBJ)
	$(CC) $(CFLAGS) -DDEBUG_MEMORY -o verify_mem verify.c $(OBJ)
clean:
	rm -f sign verify verify_bench verify_mem $(WOLFROOT)/wolfcrypt/src/*.o
size :
	size $(OBJ) sign verify 
