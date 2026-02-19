uC      = gcc
LD      = ld
OBJCOPY = objcopy
SIZE    = size

# Path to the wolfssl source tree (sibling of wolfssl-examples by default)
WOLFSSL_PATH ?= $(abspath $(CURDIR)/../../wolfssl)

# QEMU configuration
QEMU_CPU     ?= qemu64
# Use a named Intel CPU model so CPUID leaf 0 reports "GenuineIntel".
# wolfSSL's cpuid_flag() only sets AES-NI when the vendor is Intel or AMD;
# the generic "qemu64" model reports "TCGTCGTCGTCG" and fails that check
# even when +aes is passed.  "Broadwell" includes AES-NI and PCLMULQDQ by
# default and works with both KVM and TCG (accel=kvm:tcg falls back cleanly).
QEMU_CPU_HW  ?= host,migratable=off
QEMU_ACCEL   ?=

# -----------------------------------------------------------------------
# x86_64 paths / flags common to all 64-bit variants
# -----------------------------------------------------------------------
GNU_EFI_LIB_PATH     ?= /usr/lib
GNU_EFI_CRT0_X64     := $(GNU_EFI_LIB_PATH)/crt0-efi-x86_64.o
GNU_EFI_LSCRIPT_X64  := $(GNU_EFI_LIB_PATH)/elf_x86_64_efi.lds
EFI_DRIVER_TARGET_X64 := efi-bsdrv-x86_64
EFI_APP_TARGET_X64    := efi-app-x86_64

LIBGCC_X64 := $(shell $(CC) -print-libgcc-file-name)

# -----------------------------------------------------------------------
# i386 paths / flags
# -----------------------------------------------------------------------
GNU_EFI_LIB_PATH32     ?= /usr/lib32
GNU_EFI_CRT0_I32       := $(GNU_EFI_LIB_PATH32)/crt0-efi-ia32.o
GNU_EFI_LSCRIPT_I32    := $(GNU_EFI_LIB_PATH32)/elf_ia32_efi.lds
EFI_DRIVER_TARGET_I32  := efi-bsdrv-ia32
EFI_APP_TARGET_I32     := efi-app-ia32
LIBGCC_I32 := /usr/lib/gcc-cross/i686-linux-gnu/14/libgcc.a

# -----------------------------------------------------------------------
# Common CFLAGS (base)
# -----------------------------------------------------------------------
CFLAGS_COMMON := \
	-fpic -ffreestanding -fno-stack-protector -fno-stack-check \
	-fshort-wchar -mno-red-zone -maccumulate-outgoing-args \
	-DUEFI -DGNUEFI -DWOLFSSL_USER_SETTINGS -DNEED_DYNAMIC_TYPE_FIX_UEFI \
	-I. -I/usr/include/efi -I/usr/include/efi/x86_64 \
	-I$(WOLFSSL_PATH) \
	-DTARGET_X86_64_EFI

CFLAGS_HW := $(CFLAGS_COMMON) -maes -mpclmul -DUEFI_HW_ACCEL

CFLAGS_NOHW := $(CFLAGS_COMMON)

CFLAGS_I32 := \
	-m32 -fpic -ffreestanding -fno-stack-protector -fno-stack-check \
	-fshort-wchar -mno-red-zone -maccumulate-outgoing-args \
	-fno-asynchronous-unwind-tables -fno-unwind-tables \
	-DUEFI -DGNUEFI -DWOLFSSL_USER_SETTINGS -DNEED_DYNAMIC_TYPE_FIX_UEFI \
	-I. -I/usr/include/efi -I/usr/include/efi/ia32 \
	-I$(WOLFSSL_PATH) \
	-DTARGET_IA32_EFI -DEFI_FUNCTION_WRAPPER -DTARGET_IA32_EFI

# -----------------------------------------------------------------------
# wolfcrypt object list
# -----------------------------------------------------------------------
OBJS_WOLFCRYPT := \
	$(WOLFSSL_PATH)/wolfcrypt/src/aes.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/asn.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/coding.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/logging.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/cpuid.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/memory.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/rsa.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/dilithium.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/falcon.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/dh.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/kdf.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/ecc.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/misc.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/sha.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/sha256.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/sha512.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/sha3.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/hash.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/hmac.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/cmac.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/pwdbased.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/pkcs7.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/pkcs12.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/wolfmath.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/tfm.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/wc_encrypt.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/error.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/random.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/wc_port.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/wc_mlkem.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/wc_mlkem_poly.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/chacha.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/chacha20_poly1305.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/poly1305.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/curve25519.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/ed25519.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/fe_operations.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/ge_operations.o \
	src/driver.o \
	src/utility_wolf.o

# AES-NI assembly objects (hw variant only)
OBJS_HW := \
	$(WOLFSSL_PATH)/wolfcrypt/src/aes_asm.o \
	$(WOLFSSL_PATH)/wolfcrypt/src/aes_gcm_asm.o

# Test app objects
OBJS_TEST := \
	src/test_app.o

# -----------------------------------------------------------------------
# LDFLAGS helpers
# -----------------------------------------------------------------------
LDFLAGS_X64 = -shared -Bsymbolic -L$(GNU_EFI_LIB_PATH) -T$(GNU_EFI_LSCRIPT_X64)
LD_GROUP_X64 = --start-group $(GNU_EFI_CRT0_X64) $(OBJS_WOLFCRYPT) --end-group -lgnuefi -lefi $(LIBGCC_X64)
LD_GROUP_X64_HW = --start-group $(GNU_EFI_CRT0_X64) $(OBJS_WOLFCRYPT) $(OBJS_HW) --end-group -lgnuefi -lefi $(LIBGCC_X64)
LD_GROUP_X64_TEST = --start-group $(GNU_EFI_CRT0_X64) $(OBJS_TEST) --end-group -lgnuefi -lefi $(LIBGCC_X64)

LDFLAGS_I32 = -shared -Bsymbolic -L$(GNU_EFI_LIB_PATH32) -m elf_i386 -T$(GNU_EFI_LSCRIPT_I32)
LD_GROUP_I32 = --start-group $(GNU_EFI_CRT0_I32) $(OBJS_WOLFCRYPT) --end-group \
	$(GNU_EFI_LIB_PATH32)/libgnuefi.a $(GNU_EFI_LIB_PATH32)/libefi.a $(LIBGCC_I32)
LD_GROUP_I32_TEST = --start-group $(GNU_EFI_CRT0_I32) $(OBJS_TEST) --end-group \
	$(GNU_EFI_LIB_PATH32)/libgnuefi.a $(GNU_EFI_LIB_PATH32)/libefi.a $(LIBGCC_I32)

EFI_EH_FRAME  ?= -j .eh_frame
EFI_REMOVE_EH ?=

# -----------------------------------------------------------------------
# Main targets
# -----------------------------------------------------------------------
.PHONY: all clean

all: run-fallback-nohw



# -----------------------------------------------------------------------
# Pattern rules
# -----------------------------------------------------------------------
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<

# sha3.o must always have SHAKE128/256 enabled
$(WOLFSSL_PATH)/wolfcrypt/src/sha3.o: $(WOLFSSL_PATH)/wolfcrypt/src/sha3.c
	$(CC) $(CFLAGS) -DWOLFSSL_SHAKE128 -DWOLFSSL_SHAKE256 \
		-UWOLFSSL_NO_SHAKE128 -UWOLFSSL_NO_SHAKE256 -c -o $@ $<

# AES-NI assembly: compile without -DWOLFSSL_USER_SETTINGS to prevent user_settings.h
# from pulling in EFI C headers that the assembler cannot parse.  Only define the
# macros the .S file actually needs: WOLFSSL_X86_64_BUILD triggers the x86_64 path.
CFLAGS_ASM := -ffreestanding -fno-stack-protector -fpic -mno-red-zone \
	-DWOLFSSL_X86_64_BUILD -maes -mpclmul

$(WOLFSSL_PATH)/wolfcrypt/src/aes_asm.o: $(WOLFSSL_PATH)/wolfcrypt/src/aes_asm.S
	$(CC) $(CFLAGS_ASM) -c -o $@ $<

$(WOLFSSL_PATH)/wolfcrypt/src/aes_gcm_asm.o: $(WOLFSSL_PATH)/wolfcrypt/src/aes_gcm_asm.S
	$(CC) $(CFLAGS_ASM) -c -o $@ $<

# -----------------------------------------------------------------------
# EFI image rules
# -----------------------------------------------------------------------
libwolfcrypt.elf: $(OBJS_WOLFCRYPT) $(EXTRA_OBJS)
	$(LD) $(LDFLAGS) --defsym=EFI_SUBSYSTEM=11 -o $@ $(LD_GROUPS)

libwolfcrypt.efi: libwolfcrypt.elf
	@echo Creating $@
	$(OBJCOPY) -j .rodata -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc $(EFI_EH_FRAME) \
		--target=$(EFI_DRV_TGT) --subsystem=11 $(EFI_REMOVE_EH) $^ $@
	@echo Size:
	$(SIZE) $@

test.elf: $(OBJS_TEST) $(EXTRA_TEST_OBJS)
	$(LD) $(LDFLAGS) --defsym=EFI_SUBSYSTEM=10 -o $@ $(LD_GROUPS_TEST)

test.efi: test.elf
	@echo Creating $@
	$(OBJCOPY) -j .rodata -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc $(EFI_EH_FRAME) \
		--target=$(EFI_APP_TGT) --subsystem=10 $(EFI_REMOVE_EH) $^ $@
	@echo Size:
	$(SIZE) $@

# -----------------------------------------------------------------------
# install
# -----------------------------------------------------------------------
install: libwolfcrypt.efi test.efi
	mkdir -p efifs
	cp libwolfcrypt.efi efifs/
	cp test.efi efifs/
	cp startup-single.nsh efifs/startup.nsh
	cp NvVars efifs/ 2>/dev/null || true

install-dual: libwolfcrypt.efi libwolfcrypt-nohw.efi test.efi
	mkdir -p efifs
	cp libwolfcrypt.efi efifs/
	cp libwolfcrypt-nohw.efi efifs/
	cp test.efi efifs/
	cp startup.nsh efifs/
	cp NvVars efifs/ 2>/dev/null || true

# -----------------------------------------------------------------------
# lib  — x86_64 with AES-NI (hw)
# -----------------------------------------------------------------------
.PHONY: lib
lib:
	@$(MAKE) CFLAGS="$(CFLAGS_HW)" LDFLAGS="$(LDFLAGS_X64)" \
		LD_GROUPS="$(LD_GROUP_X64_HW)" LD_GROUPS_TEST="$(LD_GROUP_X64_TEST)" \
		EXTRA_OBJS="$(OBJS_HW)" EXTRA_TEST_OBJS="$(GNU_EFI_CRT0_X64)" \
		EFI_DRV_TGT="$(EFI_DRIVER_TARGET_X64)" EFI_APP_TGT="$(EFI_APP_TARGET_X64)" \
		libwolfcrypt.efi test.efi install

# -----------------------------------------------------------------------
# lib-nohw  — x86_64 software-only
# -----------------------------------------------------------------------
.PHONY: lib-nohw
lib-nohw: CFLAGS = $(CFLAGS_NOHW)
lib-nohw: LDFLAGS = $(LDFLAGS_X64)
lib-nohw: LD_GROUPS = $(LD_GROUP_X64)
lib-nohw: LD_GROUPS_TEST = $(LD_GROUP_X64_TEST)
lib-nohw: EXTRA_TEST_OBJS = $(GNU_EFI_CRT0_X64)
lib-nohw: EFI_DRV_TGT = $(EFI_DRIVER_TARGET_X64)
lib-nohw: EFI_APP_TGT = $(EFI_APP_TARGET_X64)
lib-nohw: libwolfcrypt.efi test.efi install

# -----------------------------------------------------------------------
# lib32  — i386, software-only
# -----------------------------------------------------------------------
.PHONY: lib32
lib32: CFLAGS = $(CFLAGS_I32)
lib32: LDFLAGS = $(LDFLAGS_I32)
lib32: LD_GROUPS = $(LD_GROUP_I32)
lib32: LD_GROUPS_TEST = $(LD_GROUP_I32_TEST)
lib32: EXTRA_TEST_OBJS =
lib32: EFI_DRV_TGT = $(EFI_DRIVER_TARGET_I32)
lib32: EFI_APP_TGT = $(EFI_APP_TARGET_I32)
lib32: EFI_EH_FRAME =
lib32: EFI_REMOVE_EH = --remove-section .eh_frame
lib32: libwolfcrypt.efi test.efi install

# -----------------------------------------------------------------------
# lib32-nohw  — alias for lib32 (i386 has no hw accel)
# -----------------------------------------------------------------------
.PHONY: lib32-nohw
lib32-nohw: lib32

# -----------------------------------------------------------------------
# run targets
# -----------------------------------------------------------------------
.PHONY: run
run: lib
	qemu-system-x86_64 -machine q35,accel=kvm -m 512 -net none -serial stdio \
		-display none -cpu $(QEMU_CPU_HW) \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive format=raw,file=fat:rw:./efifs \
		-object rng-random,id=rng0,filename=/dev/urandom \
		-device virtio-rng-pci,rng=rng0

.PHONY: run-nohw
run-nohw: lib-nohw
	qemu-system-x86_64 -machine q35,accel=kvm -m 512 -net none -serial stdio \
		-display none -cpu $(QEMU_CPU) \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive format=raw,file=fat:rw:./efifs \
		-object rng-random,id=rng0,filename=/dev/urandom \
		-device virtio-rng-pci,rng=rng0

.PHONY: run32
run32: lib32
	qemu-system-i386 -m 512 -machine q35,accel=kvm -net none -serial stdio \
		-display none -cpu $(QEMU_CPU_HW) \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF32_CODE_4M.fd \
		-drive format=raw,file=fat:rw:./efifs

.PHONY: run32-nohw
run32-nohw: lib32-nohw
	qemu-system-i386 -m 512 -machine q35,accel=kvm -net none -serial stdio \
		-display none -cpu qemu32 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF32_CODE_4M.fd \
		-drive format=raw,file=fat:rw:./efifs

# -----------------------------------------------------------------------
# run-fallback-nohw
#   Build hw + nohw drivers, install both; run QEMU without AES-NI so the
#   hw driver exits EFI_UNSUPPORTED and startup.nsh loads the nohw driver.
# -----------------------------------------------------------------------
.PHONY: run-fallback-nohw
run-fallback-nohw:
	@echo "=== Building hw driver (with AES-NI) ==="
	@$(MAKE) CFLAGS="$(CFLAGS_HW)" LDFLAGS="$(LDFLAGS_X64)" \
		LD_GROUPS="$(LD_GROUP_X64_HW)" LD_GROUPS_TEST="$(LD_GROUP_X64_TEST)" \
		EXTRA_OBJS="$(OBJS_HW)" EXTRA_TEST_OBJS="$(GNU_EFI_CRT0_X64)" \
		EFI_DRV_TGT="$(EFI_DRIVER_TARGET_X64)" EFI_APP_TGT="$(EFI_APP_TARGET_X64)" \
		libwolfcrypt.efi test.efi
	@echo "=== Saving hw driver ==="
	@cp libwolfcrypt.efi libwolfcrypt-hw-tmp.efi
	@echo "=== Cleaning objects ==="
	@$(MAKE) clean-objs
	@echo "=== Building nohw driver ==="
	@$(MAKE) CFLAGS="$(CFLAGS_NOHW)" LDFLAGS="$(LDFLAGS_X64)" \
		LD_GROUPS="$(LD_GROUP_X64)" LD_GROUPS_TEST="$(LD_GROUP_X64_TEST)" \
		EXTRA_TEST_OBJS="$(GNU_EFI_CRT0_X64)" \
		EFI_DRV_TGT="$(EFI_DRIVER_TARGET_X64)" EFI_APP_TGT="$(EFI_APP_TARGET_X64)" \
		libwolfcrypt.efi
	@mv libwolfcrypt.efi libwolfcrypt-nohw.efi
	@mv libwolfcrypt-hw-tmp.efi libwolfcrypt.efi
	@echo "=== Installing dual build to efifs ==="
	@mkdir -p efifs
	@cp libwolfcrypt.efi efifs/
	@cp libwolfcrypt-nohw.efi efifs/
	@cp test.efi efifs/
	@cp startup.nsh efifs/
	@cp NvVars efifs/ 2>/dev/null || true
	@echo "=== Running QEMU without AES-NI (fallback test) ==="
	qemu-system-x86_64 -m 512 -net none -serial stdio -display none \
		-cpu $(QEMU_CPU) \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive format=raw,file=fat:rw:./efifs \
		-object rng-random,id=rng0,filename=/dev/urandom \
		-device virtio-rng-pci,rng=rng0

# -----------------------------------------------------------------------
# clean
# -----------------------------------------------------------------------
.PHONY: clean
clean:
	rm -f *.elf *.efi *.o libwolfcrypt-hw-tmp.efi libwolfcrypt-nohw.efi
	rm -f $(WOLFSSL_PATH)/wolfcrypt/src/*.o src/*.o

.PHONY: clean-objs
clean-objs:
	rm -f $(WOLFSSL_PATH)/wolfcrypt/src/*.o src/*.o

.PHONY: all
all: lib-nohw
