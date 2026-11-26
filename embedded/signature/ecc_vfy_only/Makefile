# The path to the wolfssl directory must be set correctly for your environment.
WOLFROOT = ../../../../wolfssl

CFLAGS = $(EX_CFLAGS) -DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT) -Os
ASFLAGS = -DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT)

OBJ=\
	$(WOLFROOT)/wolfcrypt/src/ecc.o\
	$(WOLFROOT)/wolfcrypt/src/sha256.o\
	$(WOLFROOT)/wolfcrypt/src/hash.o\
	$(WOLFROOT)/wolfcrypt/src/random.o\
	$(WOLFROOT)/wolfcrypt/src/asn.o\
	$(WOLFROOT)/wolfcrypt/src/wc_port.o\
	$(WOLFROOT)/wolfcrypt/src/coding.o\
	$(WOLFROOT)/wolfcrypt/src/memory.o\
	$(WOLFROOT)/wolfcrypt/src/wolfmath.o\
	$(WOLFROOT)/wolfcrypt/src/wc_encrypt.o\

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
	$(WOLFROOT)/wolfcrypt/src/tfm.o\

.PHONY: all clean mem size bench
# Every target depends on "clean", which deletes the wolfCrypt objects.
# Running in parallel would race with that, so force serial execution.
.NOTPARALLEL:

# Memory tracking (DEBUG_MEMORY) also has to be enabled in memory.c, so
# compile it together with the example instead of linking memory.o.
OBJ_MEM = $(WOLFROOT)/wolfcrypt/src/memory.c \
	$(filter-out $(WOLFROOT)/wolfcrypt/src/memory.o,$(OBJ))

ifeq ($(math) $(arch),sp x64)
ASFLAGS+= -DSP_X86_64_FLAG
CFLAGS += -DSP_X86_64_FLAG
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

all : ecc_verify bench mem


ecc_verify: clean $(OBJ)
	$(CC) $(CFLAGS) -o ecc_verify ecc_verify.c $(OBJ)

bench: clean $(OBJ)
	$(CC) $(CFLAGS) -DBENCHMARK -o ecc_verify_bench ecc_verify.c $(OBJ)

mem: clean $(OBJ)
	$(CC) $(CFLAGS) -DDEBUG_MEMORY -o ecc_verify_mem ecc_verify.c $(OBJ_MEM) -lpthread
clean:
	rm -f  ecc_verify ecc_verify_bench ecc_verify_mem $(WOLFROOT)/wolfcrypt/src/*.o

size :
	size $(OBJ) ecc_verify
