SDK?=./SDK-2.8.2_EVK-MIMXRT1060
WOLFSSL?=../../wolfssl

CC=arm-none-eabi-gcc

ASMFLAGS=-D__STARTUP_CLEAR_BSS -D__STARTUP_INITIALIZE_NONCACHEDATA -mcpu=cortex-m7 -Wall -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99
CFLAGS=-DXIP_EXTERNAL_FLASH=1 -DXIP_BOOT_HEADER_ENABLE=1 -DCPU_MIMXRT1062DVL6A -DPRINTF_FLOAT_ENABLE=1 -DSCANF_FLOAT_ENABLE=1 -DPRINTF_ADVANCED_ENABLE=1 -DSCANF_ADVANCED_ENABLE=1 -DSERIAL_PORT_TYPE_UART=1 -Os -mcpu=cortex-m7 -Wall -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb -MMD -MP -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mapcs -std=gnu99 -DXPRINTF=PRINTF
LDFLAGS= -mcpu=cortex-m7 -Wall -mfloat-abi=hard -mfpu=fpv5-d16 --specs=nosys.specs -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mthumb -mapcs -Xlinker --gc-sections -Xlinker -static -Xlinker -z -Xlinker muldefs -Xlinker -Map=output.map -T MIMXRT1062xxxxx_flexspi_nor.ld -static -lm -lc -lnosys

CFLAGS+=-I$(SDK)/devices/MIMXRT1062/utilities/debug_console/
CFLAGS+=-I$(SDK)/components/serial_manager -I$(SDK)/components/uart/
CFLAGS+=-I$(SDK) -I$(SDK)/devices/MIMXRT1062/drivers/ -I$(SDK)/devices/MIMXRT1062 -I$(SDK)/CMSIS/Include
CFLAGS+=-I$(SDK)/devices/MIMXRT1062/utilities/str
CFLAGS+=-I. -I$(WOLFSSL)

CFLAGS+=-DWOLFSSL_USER_SETTINGS


OBJS=common.o board.o pin_mux.o dcd.o clock_config.o $(SDK)/devices/MIMXRT1062/gcc/startup_MIMXRT1062.o $(SDK)/devices/MIMXRT1062/system_MIMXRT1062.o \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_clock.o \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_trng.o \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_common.o \
     $(SDK)/devices/MIMXRT1062/utilities/debug_console/fsl_debug_console.o \
     $(SDK)/devices/MIMXRT1062/utilities/str/fsl_str.o \
     $(SDK)/components/uart/lpuart_adapter.o \
     $(SDK)/components/serial_manager/serial_manager.o \
     $(SDK)/components/lists/generic_list.o \
     $(SDK)/components/serial_manager/serial_port_uart.o \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_lpuart.o \
     $(SDK)/devices/MIMXRT1062/utilities/fsl_assert.o \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_gpio.o \
     $(SDK)/devices/MIMXRT1062/xip/fsl_flexspi_nor_boot.o \
     $(SDK)/boards/evkmimxrt1060/xip/evkmimxrt1060_flexspi_nor_config.o \
     $(SDK)/devices/MIMXRT1062/utilities/fsl_sbrk.o

OBJS+= \
     $(WOLFSSL)/wolfcrypt/src/wc_port.o \
     $(WOLFSSL)/wolfcrypt/src/wc_encrypt.o \
     $(WOLFSSL)/wolfcrypt/src/random.o \
     $(WOLFSSL)/wolfcrypt/src/logging.o \
     $(WOLFSSL)/wolfcrypt/src/sp_int.o \
     $(WOLFSSL)/wolfcrypt/src/sp_cortexm.o \
     $(WOLFSSL)/wolfcrypt/src/ecc.o \
     $(WOLFSSL)/wolfcrypt/src/rsa.o \
     $(WOLFSSL)/wolfcrypt/src/aes.o \
     $(WOLFSSL)/wolfcrypt/src/asn.o \
     $(WOLFSSL)/wolfcrypt/src/sha.o \
     $(WOLFSSL)/wolfcrypt/src/sha256.o \
     $(WOLFSSL)/wolfcrypt/src/sha512.o \
     $(WOLFSSL)/wolfcrypt/src/sha3.o \
     $(WOLFSSL)/wolfcrypt/src/chacha.o \
     $(WOLFSSL)/wolfcrypt/src/ed25519.o \
     $(WOLFSSL)/wolfcrypt/src/md5.o \
     $(WOLFSSL)/wolfcrypt/src/hmac.o \
     $(WOLFSSL)/wolfcrypt/src/coding.o \
     $(WOLFSSL)/wolfcrypt/src/ge_low_mem.o \
     $(WOLFSSL)/wolfcrypt/src/hash.o \
     $(WOLFSSL)/wolfcrypt/src/pwdbased.o \
     $(WOLFSSL)/wolfcrypt/src/wolfmath.o \
     $(WOLFSSL)/wolfcrypt/src/fe_low_mem.o

ifneq ($(DCP),0)
    OBJS+= \
     $(SDK)/devices/MIMXRT1062/drivers/fsl_dcp.o \
     $(WOLFSSL)/wolfcrypt/src/port/nxp/dcp_port.o
    CFLAGS+=-DWOLFSSL_IMXRT_DCP
endif	

TEST_OBJS:=$(WOLFSSL)/wolfcrypt/test/test.o main-test.o
BENCH_OBJS:=$(WOLFSSL)/wolfcrypt/benchmark/benchmark.o main-bench.o

all: wolfcrypt-test.bin wolfcrypt-benchmark.bin

wolfcrypt-test.bin: wolfcrypt-test.elf
	arm-none-eabi-objcopy -O binary $^ $@

wolfcrypt-benchmark.bin: wolfcrypt-benchmark.elf
	arm-none-eabi-objcopy -O binary $^ $@

wolfcrypt-test.elf: $(OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

wolfcrypt-benchmark.elf: $(OBJS) $(BENCH_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o *.elf *.bin *.map *.d
	rm -f $(WOLFSSL)/wolfcrypt/src/*.o
	rm -f $(WOLFSSL)/wolfcrypt/benchmark/*.o
	rm -f $(WOLFSSL)/wolfcrypt/test/*.o
	rm -f $(WOLFSSL)/wolfcrypt/port/nxp/*.o
