# The path to the wolfssl directory must be set correctly for your environment.
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


.PHONY: all clean size mem


CFLAGS += -DTFM_FLAG
OBJ += $(OBJ_TFM)


all: verify_nonblock mem

verify_nonblock: clean $(OBJ)
	$(CC) $(CFLAGS) -o verify_nonblock verify_nonblock.c $(OBJ) -lpthread

mem: clean $(OBJ)
	$(CC) $(CFLAGS) -DDEBUG_MEMORY -o verify_mem_nonblock verify_nonblock.c $(OBJ) -lpthread
clean:
	rm -f verify_nonblock verify_mem_nonblock $(WOLFROOT)/wolfcrypt/src/*.o
size :
	size $(OBJ) verify
