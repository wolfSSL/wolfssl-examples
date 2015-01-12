################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
utils/uartstdio.obj: C:/Users/khimes/workspace_v6_testWorkspace/utils/uartstdio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="utils/uartstdio.pp" --obj_directory="utils" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


