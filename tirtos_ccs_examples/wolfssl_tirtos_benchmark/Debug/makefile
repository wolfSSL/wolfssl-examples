################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := c:/ti/ccsv6/tools/compiler/arm_5.1.6

ORDERED_OBJS += \
$(GEN_CMDS__FLAG) \
"./wolfssl/ctaocrypt/src/tfm.obj" \
"./wolfssl/ctaocrypt/src/sha512.obj" \
"./wolfssl/ctaocrypt/src/sha256.obj" \
"./wolfssl/ctaocrypt/src/sha.obj" \
"./wolfssl/ctaocrypt/src/rsa.obj" \
"./wolfssl/ctaocrypt/src/rabbit.obj" \
"./wolfssl/ctaocrypt/src/poly1305.obj" \
"./wolfssl/ctaocrypt/src/md5.obj" \
"./wolfssl/ctaocrypt/src/integer.obj" \
"./wolfssl/ctaocrypt/src/hc128.obj" \
"./wolfssl/ctaocrypt/src/ecc.obj" \
"./wolfssl/ctaocrypt/src/dh.obj" \
"./wolfssl/ctaocrypt/src/des3.obj" \
"./wolfssl/ctaocrypt/src/chacha.obj" \
"./wolfssl/ctaocrypt/src/camellia.obj" \
"./wolfssl/ctaocrypt/src/blake2b.obj" \
"./wolfssl/ctaocrypt/src/asn.obj" \
"./wolfssl/ctaocrypt/src/asm.obj" \
"./wolfssl/ctaocrypt/src/arc4.obj" \
"./wolfssl/ctaocrypt/src/aes.obj" \
"./wolfssl_tirtos_benchmark.obj" \
"./benchmark.obj" \
"./EK_TM4C1294XL.obj" \
"../EK_TM4C1294XL.cmd" \
-l"libc.a" \
-l"c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c/driverlib/ccs/Debug/driverlib.lib" \
-l"c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c/usblib/ccs/Debug/usblib.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include wolfssl/ctaocrypt/src/subdir_vars.mk
-include subdir_rules.mk
-include wolfssl/ctaocrypt/src/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
wolfssl_tirtos_benchmark.out \

EXE_OUTPUTS__QUOTED += \
"wolfssl_tirtos_benchmark.out" \

BIN_OUTPUTS += \
wolfssl_tirtos_benchmark.hex \

BIN_OUTPUTS__QUOTED += \
"wolfssl_tirtos_benchmark.hex" \


# All Target
all: wolfssl_tirtos_benchmark.out

# Tool invocations
wolfssl_tirtos_benchmark.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Linker'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O3 --opt_for_speed=5 -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_ECC --define=CYASSL_SHA512 --define=HAVE_CHACHA --define=HAVE_POLY1305 --define=HAVE_CAMELLIA --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --optimizer_interlist --remove_hooks_when_inlining --opt_for_cache --gen_opt_info=2 --call_assumptions=3 --auto_inline=5 --single_inline --aliased_variables -z -m"wolfssl_tirtos_benchmark.map" --heap_size=0 --stack_size=512 -i"c:/ti/ccsv6/tools/compiler/arm_5.1.6/lib" -i"c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --reread_libs --warn_sections --display_error_number --diag_wrap=off --xml_link_info="wolfssl_tirtos_benchmark_linkInfo.xml" --rom_model -o "wolfssl_tirtos_benchmark.out" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

wolfssl_tirtos_benchmark.hex: $(EXE_OUTPUTS)
	@echo 'Invoking: ARM Hex Utility'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armhex"  -o "wolfssl_tirtos_benchmark.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)$(GEN_FILES__QUOTED)$(BIN_OUTPUTS__QUOTED)$(GEN_OPTS__QUOTED)$(GEN_CMDS__QUOTED)
	-$(RMDIR) $(GEN_MISC_DIRS__QUOTED)
	-$(RM) "EK_TM4C1294XL.pp" "benchmark.pp" "wolfssl_tirtos_benchmark.pp" "wolfssl\ctaocrypt\src\aes.pp" "wolfssl\ctaocrypt\src\arc4.pp" "wolfssl\ctaocrypt\src\asm.pp" "wolfssl\ctaocrypt\src\asn.pp" "wolfssl\ctaocrypt\src\blake2b.pp" "wolfssl\ctaocrypt\src\camellia.pp" "wolfssl\ctaocrypt\src\chacha.pp" "wolfssl\ctaocrypt\src\des3.pp" "wolfssl\ctaocrypt\src\dh.pp" "wolfssl\ctaocrypt\src\ecc.pp" "wolfssl\ctaocrypt\src\hc128.pp" "wolfssl\ctaocrypt\src\integer.pp" "wolfssl\ctaocrypt\src\md5.pp" "wolfssl\ctaocrypt\src\poly1305.pp" "wolfssl\ctaocrypt\src\rabbit.pp" "wolfssl\ctaocrypt\src\rsa.pp" "wolfssl\ctaocrypt\src\sha.pp" "wolfssl\ctaocrypt\src\sha256.pp" "wolfssl\ctaocrypt\src\sha512.pp" "wolfssl\ctaocrypt\src\tfm.pp" 
	-$(RM) "EK_TM4C1294XL.obj" "benchmark.obj" "wolfssl_tirtos_benchmark.obj" "wolfssl\ctaocrypt\src\aes.obj" "wolfssl\ctaocrypt\src\arc4.obj" "wolfssl\ctaocrypt\src\asm.obj" "wolfssl\ctaocrypt\src\asn.obj" "wolfssl\ctaocrypt\src\blake2b.obj" "wolfssl\ctaocrypt\src\camellia.obj" "wolfssl\ctaocrypt\src\chacha.obj" "wolfssl\ctaocrypt\src\des3.obj" "wolfssl\ctaocrypt\src\dh.obj" "wolfssl\ctaocrypt\src\ecc.obj" "wolfssl\ctaocrypt\src\hc128.obj" "wolfssl\ctaocrypt\src\integer.obj" "wolfssl\ctaocrypt\src\md5.obj" "wolfssl\ctaocrypt\src\poly1305.obj" "wolfssl\ctaocrypt\src\rabbit.obj" "wolfssl\ctaocrypt\src\rsa.obj" "wolfssl\ctaocrypt\src\sha.obj" "wolfssl\ctaocrypt\src\sha256.obj" "wolfssl\ctaocrypt\src\sha512.obj" "wolfssl\ctaocrypt\src\tfm.obj" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

