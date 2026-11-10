# Makefile for the wolfCrypt + wolfHAL example
#
# Usage:
#   make
#   make BOARD=stm32wb55xx_nucleo
#   make flash

BOARD ?= stm32wb55xx_nucleo

WOLFSSL_ROOT ?= $(abspath ../../wolfssl)
WHAL_DIR     ?= $(abspath wolfHAL)
GCC_PATH     ?=

TARGET = wolfcrypt_test
IMAGE ?= $(TARGET).bin

# Toolchain, the direct-API-mapping selection, the wolfHAL drivers this board
# needs (BOARD_SOURCE) and the linker script.
include boards/$(BOARD)/board.mk

CFLAGS += -I. -I$(WHAL_DIR) -I$(WOLFSSL_ROOT) -DWOLFSSL_USER_SETTINGS

# Application
SRC  = main.c
SRC += syscalls.c

# Board and wolfHAL drivers
SRC += $(BOARD_SOURCE)

# wolfCrypt core
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/wc_port.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/memory.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/aes.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/cryptocb.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/sha256.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/hash.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/random.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/logging.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/error.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/wc_encrypt.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/sp_int.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/test/test.c
SRC += $(WOLFSSL_ROOT)/wolfcrypt/benchmark/benchmark.c

# wolfSSL's wolfHAL port
SRC += $(WOLFSSL_ROOT)/wolfcrypt/src/port/wolfHAL/wolfhal.c

# Objects go under build/ with their path flattened into the name. Compiling in
# place would drop ARM objects into the wolfSSL and wolfHAL checkouts, where
# they collide with those trees' own builds.
OBJDIR = build
objname = $(OBJDIR)/$(subst /,_,$(patsubst /%,%,$(basename $(1)))).o
OBJ = $(foreach s,$(SRC),$(call objname,$(s)))

all: $(TARGET).bin
	$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJ)
	$(GCC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(OBJDIR):
	mkdir -p $@

define compile_rule
$(call objname,$(1)): $(1) | $(OBJDIR)
	$$(GCC) $$(CFLAGS) -c -o $$@ $$<
endef
$(foreach s,$(SRC),$(eval $(call compile_rule,$(s))))

# Flash via openocd (ST-LINK on the Nucleo).
flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32wbx.cfg \
	        -c "program $(TARGET).bin 0x08000000 verify reset exit"

clean:
	rm -rf $(OBJDIR) $(TARGET).elf $(TARGET).bin

# Query a variable, e.g. `make -s BOARD=x print-CROSS_COMPILE`. Lets CI ask the
# board for its toolchain rather than keeping a second copy of that mapping.
print-%:
	@echo "$($*)"

.PHONY: all flash clean

-include $(OBJ:.o=.d)
