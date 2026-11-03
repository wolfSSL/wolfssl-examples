# wolfCrypt test and benchmark for the NXP LPC55S6x (LPCXpresso55S69)
#
# MCUXSDK must point at an mcuxsdk-core checkout that west has populated, so
# that devices/, examples/ and components/ are present alongside drivers/:
#
#   west init -m https://github.com/nxp-mcuxpresso/mcuxsdk-manifests mcuxsdk-ws
#   cd mcuxsdk-ws && west update
#   make MCUXSDK=/path/to/mcuxsdk-ws/mcuxsdk

MCUXSDK ?=
WOLFSSL ?= ../../../wolfssl

ifeq ($(MCUXSDK),)
  ifneq ($(MAKECMDGOALS),clean)
    $(error Set MCUXSDK to your mcuxsdk-core checkout, e.g. make MCUXSDK=~/mcuxsdk-ws/mcuxsdk)
  endif
endif

# Set to 0 to build the same algorithms in software, for comparison.
HW_CRYPTO ?= 1

CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE    = $(CROSS_COMPILE)size

BUILD  = build
TARGET = wolfcrypt-lpc55s6x

DEVICE  = $(MCUXSDK)/devices/LPC/LPC5500/LPC55S69
BOARDIR = $(MCUXSDK)/examples/_boards/lpcxpresso55s69

CPUFLAGS = -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16

DEFINES = \
	-DCPU_LPC55S69JBD100_cm33_core0 \
	-DMCUXPRESSO_SDK \
	-DSDK_DEBUGCONSOLE=1 \
	-DPRINTF_FLOAT_ENABLE=1 \
	-DPRINTF_ADVANCED_ENABLE=1 \
	-DSERIAL_PORT_TYPE_UART=1 \
	-DWOLFSSL_USER_SETTINGS

INCLUDES = \
	-I. \
	-I$(WOLFSSL) \
	-I$(MCUXSDK)/arch/arm/CMSIS/Core/Include \
	-I$(DEVICE) \
	-I$(MCUXSDK)/devices/LPC/LPC5500/periph \
	-I$(DEVICE)/drivers \
	-I$(MCUXSDK)/drivers/common \
	-I$(MCUXSDK)/drivers/flexcomm \
	-I$(MCUXSDK)/drivers/flexcomm/usart \
	-I$(MCUXSDK)/drivers/flexcomm/i2c \
	-I$(MCUXSDK)/drivers/lpc_gpio \
	-I$(MCUXSDK)/drivers/lpc_iocon \
	-I$(MCUXSDK)/drivers/casper \
	-I$(MCUXSDK)/drivers/hashcrypt \
	-I$(MCUXSDK)/drivers/rng_1 \
	-I$(MCUXSDK)/components/uart \
	-I$(MCUXSDK)/components/debug_console_lite \
	-I$(MCUXSDK)/components/str \
	-I$(BOARDIR) \
	-I$(BOARDIR)/project_template

CFLAGS = $(CPUFLAGS) $(DEFINES) $(INCLUDES) \
	-Os -g3 -std=gnu99 -Wall \
	-ffunction-sections -fdata-sections -fno-common \
	-ffreestanding -fno-builtin -MMD -MP

ASFLAGS = $(CPUFLAGS) -D__STARTUP_CLEAR_BSS -x assembler-with-cpp

LDFLAGS = $(CPUFLAGS) \
	-T$(DEVICE)/gcc/LPC55S69_cm33_core0_flash.ld \
	--specs=nosys.specs --specs=nano.specs -u _printf_float \
	-Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
	-Wl,--defsym=__stack_size__=0x4000 \
	-Wl,--defsym=__heap_size__=0x8000
LDLIBS = -lm -lc -lnosys

# --- SDK -------------------------------------------------------------------
SDK_SRCS = \
	$(DEVICE)/system_LPC55S69_cm33_core0.c \
	$(DEVICE)/drivers/fsl_clock.c \
	$(DEVICE)/drivers/fsl_power.c \
	$(DEVICE)/drivers/fsl_reset.c \
	$(MCUXSDK)/drivers/common/fsl_common.c \
	$(MCUXSDK)/drivers/common/fsl_common_arm.c \
	$(MCUXSDK)/drivers/flexcomm/fsl_flexcomm.c \
	$(MCUXSDK)/drivers/flexcomm/usart/fsl_usart.c \
	$(MCUXSDK)/drivers/rng_1/fsl_rng.c \
	$(MCUXSDK)/components/uart/fsl_adapter_usart.c \
	$(MCUXSDK)/components/debug_console_lite/fsl_debug_console.c \
	$(MCUXSDK)/components/str/fsl_str.c \
	$(BOARDIR)/board.c \
	$(BOARDIR)/clock_config.c \
	$(BOARDIR)/project_template/pin_mux.c

ASM_SRCS = $(DEVICE)/gcc/startup_LPC55S69_cm33_core0.S

# --- wolfSSL ---------------------------------------------------------------
WOLF_SRCS = \
	$(WOLFSSL)/wolfcrypt/src/wc_port.c \
	$(WOLFSSL)/wolfcrypt/src/memory.c \
	$(WOLFSSL)/wolfcrypt/src/logging.c \
	$(WOLFSSL)/wolfcrypt/src/error.c \
	$(WOLFSSL)/wolfcrypt/src/wolfmath.c \
	$(WOLFSSL)/wolfcrypt/src/hash.c \
	$(WOLFSSL)/wolfcrypt/src/hmac.c \
	$(WOLFSSL)/wolfcrypt/src/sha.c \
	$(WOLFSSL)/wolfcrypt/src/sha256.c \
	$(WOLFSSL)/wolfcrypt/src/aes.c \
	$(WOLFSSL)/wolfcrypt/src/wc_encrypt.c \
	$(WOLFSSL)/wolfcrypt/src/rsa.c \
	$(WOLFSSL)/wolfcrypt/src/asn.c \
	$(WOLFSSL)/wolfcrypt/src/coding.c \
	$(WOLFSSL)/wolfcrypt/src/kdf.c \
	$(WOLFSSL)/wolfcrypt/src/random.c \
	$(WOLFSSL)/wolfcrypt/src/sp_int.c \
	$(WOLFSSL)/wolfcrypt/src/sp_cortexm.c

APP_SRCS = main.c \
	$(WOLFSSL)/wolfcrypt/test/test.c \
	$(WOLFSSL)/wolfcrypt/benchmark/benchmark.c

ifeq ($(HW_CRYPTO),0)
DEFINES += -DWOLFSSL_LPC55S6X_SOFTWARE_ONLY
else
SDK_SRCS += \
	$(MCUXSDK)/drivers/casper/fsl_casper.c \
	$(MCUXSDK)/drivers/hashcrypt/fsl_hashcrypt.c
WOLF_SRCS += \
	$(WOLFSSL)/wolfcrypt/src/port/nxp/casper_port.c \
	$(WOLFSSL)/wolfcrypt/src/port/nxp/hashcrypt_port.c
endif

SRCS = $(SDK_SRCS) $(WOLF_SRCS) $(APP_SRCS)
OBJS = $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o)) $(notdir $(ASM_SRCS:.S=.o)))
VPATH = $(sort $(dir $(SRCS) $(ASM_SRCS)))

.PHONY: all clean
all: $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@$(SIZE) $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.S | $(BUILD)
	$(CC) $(ASFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
