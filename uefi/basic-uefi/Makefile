BUILD_DIR=./build
CRYPTO_SRC=./wolfssl/wolfcrypt/src
WOLFSSL_REPO=https://github.com/wolfSSL/wolfssl
CFLAGS=-fpie -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -static-libgcc -nostdlib
# enable user settings
CFLAGS+= -DWOLFSSL_USER_SETTINGS
CFLAGS+=-I./wolfssl/ -I. -I/usr/include/efi
CFLAGS+=-ggdb
CC=gcc
LD=ld
LDFLAGS_START=-static -pie --no-dynamic-linker -Bsymbolic -L/usr/lib -L/usr/lib/gcc/x86_64-pc-linux-gnu/13.2.1/ -Lgnu-efi-dir/x86_64/gnuefi -T/usr/lib/elf_x86_64_efi.lds /usr/lib/crt0-efi-x86_64.o $(LDFLAGS)
LDFLAGS_END=-lgnuefi -lefi -lgcc

_OBJS=sha256.o sha3.o misc.o coding.o hmac.o rsa.o random.o wolfmath.o		\
	integer.o tfm.o asm.o cpuid.o memory.o logging.o wc_port.o asn.o hash.o	\
	main.o test.o error.o string.o aes.o wc_encrypt.o

OBJS=$(addprefix $(BUILD_DIR)/,$(_OBJS))

$(shell mkdir -p $(BUILD_DIR))

all: wolfcrypt.efi

.PHONY: clone_repo

clone_repo:
	@if [ ! -d $(CRYPTO_SRC) ]; then \
	    git clone $(WOLFSSL_REPO);	 \
	fi

create_build_dir:
	@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

$(BUILD_DIR)/wolfcrypt.elf: clone_repo $(OBJS)
	$(LD) $(LDFLAGS_START) $(OBJS) -o $(BUILD_DIR)/wolfcrypt.elf $(LDFLAGS_END)

wolfcrypt.efi: $(BUILD_DIR)/wolfcrypt.elf
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 $(BUILD_DIR)/wolfcrypt.elf wolfcrypt.efi

$(BUILD_DIR)/test.o: $(CRYPTO_SRC)/../test/test.c
	$(CC) $(CFLAGS) -c $? -o $@

$(BUILD_DIR)/main.o: main.c
	$(CC) $(CFLAGS) -c $? -o $@

$(BUILD_DIR)/string.o: string.c
	$(CC) $(CFLAGS) -c $? -o $@

$(BUILD_DIR)/%.o: $(CRYPTO_SRC)/%.c
	$(CC) $(CFLAGS) -c $? -o $@

clean:
	rm -rf $(BUILD_DIR) wolfcrypt.efi
