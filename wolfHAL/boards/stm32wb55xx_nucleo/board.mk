# board.mk — STM32WB55 Nucleo
#
# Toolchain, the direct-API-mapping selection, and the wolfHAL drivers this
# board needs. Only the peripherals the wolfCrypt test uses are built.

_BOARD_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CROSS_COMPILE = arm-none-eabi-

GCC     = $(GCC_PATH)$(CROSS_COMPILE)gcc
OBJCOPY = $(GCC_PATH)$(CROSS_COMPILE)objcopy
SIZE    = $(GCC_PATH)$(CROSS_COMPILE)size

# Each WHAL_CFG_*_DIRECT_API_MAPPING binds a generic whal_<X>_* entry point
# straight to this part's driver, so there is no vtable indirection at runtime.
# Drop one and that subsystem falls back to pointer dispatch, which needs the
# matching .driver field in the wolfHAL_board.h device initializer.
CFLAGS += -Wall -Werror -g3 -Os \
          -ffreestanding -nostdlib -mcpu=cortex-m4 -mthumb \
          -ffunction-sections -fdata-sections \
          -MMD -MP \
          -DPLATFORM_STM32WB \
          -DWHAL_CFG_STM32WB_RCC_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_GPIO_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_UART_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_RNG_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_INIT_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_ECB_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_CBC_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_CTR_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_GCM_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_GMAC_DIRECT_API_MAPPING \
          -DWHAL_CFG_STM32WB_AES_CCM_DIRECT_API_MAPPING \
          -DWHAL_CFG_SYSTICK_TIMER_DIRECT_API_MAPPING

LINKER_SCRIPT ?= $(_BOARD_DIR)/linker.ld

# Link through gcc rather than ld so newlib is available for the test suite's
# printf. -lnosys resolves the stubs syscalls.c does not provide.
LDFLAGS += -nostdlib -mcpu=cortex-m4 -mthumb -Wl,--gc-sections -T $(LINKER_SCRIPT)
LDLIBS  += -Wl,--start-group -lc -lm -lgcc -lnosys -Wl,--end-group

# wolfHAL_board.h has to be reachable by quoted include: wolfHAL's driver TUs
# and wolfSSL's wolfhal.c both pull it in that way.
CFLAGS += -I$(_BOARD_DIR)

BOARD_SOURCE  = $(_BOARD_DIR)/wolfHAL_board.c
BOARD_SOURCE += $(_BOARD_DIR)/ivt.c

# wolfHAL drivers
BOARD_SOURCE += $(WHAL_DIR)/src/reg.c
BOARD_SOURCE += $(WHAL_DIR)/src/gpio/stm32wb_gpio.c
BOARD_SOURCE += $(WHAL_DIR)/src/uart/stm32wb_uart.c
BOARD_SOURCE += $(WHAL_DIR)/src/rng/stm32wb_rng.c
BOARD_SOURCE += $(WHAL_DIR)/src/crypto/stm32wb_aes.c
BOARD_SOURCE += $(WHAL_DIR)/src/flash/stm32wb_flash.c
BOARD_SOURCE += $(WHAL_DIR)/src/timer/systick.c
