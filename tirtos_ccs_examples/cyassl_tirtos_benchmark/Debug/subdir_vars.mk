################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../EK_TM4C1294XL.cmd 

CFG_SRCS += \
../cyassl_tirtos_benchmark.cfg 

C_SRCS += \
../EK_TM4C1294XL.c \
C:/cyassl/ctaocrypt/benchmark/benchmark.c \
../cyassl_tirtos_benchmark.c 

OBJS += \
./EK_TM4C1294XL.obj \
./benchmark.obj \
./cyassl_tirtos_benchmark.obj 

C_DEPS += \
./EK_TM4C1294XL.pp \
./benchmark.pp \
./cyassl_tirtos_benchmark.pp 

GEN_MISC_DIRS += \
./configPkg/ 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_OPTS += \
./configPkg/compiler.opt 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

C_DEPS__QUOTED += \
"EK_TM4C1294XL.pp" \
"benchmark.pp" \
"cyassl_tirtos_benchmark.pp" 

OBJS__QUOTED += \
"EK_TM4C1294XL.obj" \
"benchmark.obj" \
"cyassl_tirtos_benchmark.obj" 

C_SRCS__QUOTED += \
"../EK_TM4C1294XL.c" \
"C:/cyassl/ctaocrypt/benchmark/benchmark.c" \
"../cyassl_tirtos_benchmark.c" 

GEN_CMDS__FLAG += \
-l"./configPkg/linker.cmd" 

GEN_OPTS__FLAG += \
--cmd_file="./configPkg/compiler.opt" 


