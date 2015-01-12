################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
wolfssl/wolfcrypt/src/asn.obj: C:/wolfssl/wolfcrypt/src/asn.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/asn.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/blake2b.obj: C:/wolfssl/wolfcrypt/src/blake2b.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/blake2b.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/camellia.obj: C:/wolfssl/wolfcrypt/src/camellia.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/camellia.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/chacha.obj: C:/wolfssl/wolfcrypt/src/chacha.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/chacha.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/ecc.obj: C:/wolfssl/wolfcrypt/src/ecc.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/ecc.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/integer.obj: C:/wolfssl/wolfcrypt/src/integer.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/integer.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/poly1305.obj: C:/wolfssl/wolfcrypt/src/poly1305.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/poly1305.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/sha512.obj: C:/wolfssl/wolfcrypt/src/sha512.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/sha512.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/wolfcrypt/src/tfm.obj: C:/wolfssl/wolfcrypt/src/tfm.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/wolfcrypt/src/tfm.pp" --obj_directory="wolfssl/wolfcrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


