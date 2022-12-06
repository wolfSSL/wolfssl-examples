WOLFROOT = ../../../../wolfssl

CFLAGS = -DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT) -Os
ASFLAGS=-DWOLFSSL_USER_SETTINGS -I. -I$(WOLFROOT)

OBJ =\
	$(WOLFROOT)/wolfcrypt/src/rsa.o\
	$(WOLFROOT)/wolfcrypt/src/sha256.o\
	$(WOLFROOT)/wolfcrypt/src/hash.o\
	$(WOLFROOT)/wolfcrypt/src/random.o\
	$(WOLFROOT)/wolfcrypt/src/tfm.o\
	$(WOLFROOT)/wolfcrypt/src/memory.o\
	$(WOLFROOT)/wolfcrypt/src/sp_int.o\
	$(WOLFROOT)/wolfcrypt/src/sp_arm64.o\
	$(WOLFROOT)/wolfcrypt/src/cpuid.o\
	$(WOLFROOT)/wolfcrypt/src/sp_x86_64.o\
	$(WOLFROOT)/wolfcrypt/src/sp_x86_64_asm.o\
	$(WOLFROOT)/wolfcrypt/src/sp_c32.o\
	$(WOLFROOT)/wolfcrypt/src/tfm.o\
	
.PHONY: all clean size sp sp_arm64 sp_x86_64 tfm debug_memory

sp:CFLAGS+= -DSP_FLAG
sp: all

sp_x86_64:CFLAGS+= -DSP_X86_64_FLAG
sp_x86_64:ASFLAGS+= -DSP_X86_64_FLAG
sp_x86_64:all

sp_arm64:CFLAGS+= -DSP_ARM64_FLAG 
sp_arm64: all

tfm:CFLAGS+= -DTFM_FLAG
tfm: all

debug_memory:CFLAGS+= -DDEBUG_MEMORY
debug_memory: verify

all: verify benchmark
verify: $(OBJ)
	$(CC) $(CFLAGS) -o verify verify.c $(OBJ)

benchmark: $(OBJ)
	$(CC) $(CFLAGS) -DBENCHMARK -o benchmark verify.c $(OBJ)
clean:
	rm -f verify benchmark $(OBJ)
size :
	size $(OBJ)
