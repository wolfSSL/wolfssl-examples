################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
wolfssl/ctaocrypt/src/asn.obj: C:/wolfssl/ctaocrypt/src/asn.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/asn.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/blake2b.obj: C:/wolfssl/ctaocrypt/src/blake2b.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/blake2b.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/camellia.obj: C:/wolfssl/ctaocrypt/src/camellia.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/camellia.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/chacha.obj: C:/wolfssl/ctaocrypt/src/chacha.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/chacha.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/ecc.obj: C:/wolfssl/ctaocrypt/src/ecc.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/ecc.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/integer.obj: C:/wolfssl/ctaocrypt/src/integer.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/integer.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/poly1305.obj: C:/wolfssl/ctaocrypt/src/poly1305.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/poly1305.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/sha512.obj: C:/wolfssl/ctaocrypt/src/sha512.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/sha512.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wolfssl/ctaocrypt/src/tfm.obj: C:/wolfssl/ctaocrypt/src/tfm.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"c:/ti/ccsv6/tools/compiler/arm_5.1.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O1 --include_path="C:/wolfssl" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc" --include_path="C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc/bsd" --include_path="c:/ti/ccsv6/tools/compiler/arm_5.1.6/include" --include_path="c:/ti/tirtos_tivac_2_00_02_36/products/TivaWare_C_Series-2.1.0.12573c" -g --gcc --define=PART_TM4C1294NCPDT --define=TIRTOS --define=ccs --define=TIVAWARE --define=_INCLUDE_NIMU_CODE --define=NO_MAIN_DRIVER --define=BENCH_EMBEDDED --define=HAVE_CAMELLIA --define=HAVE_CHACHA --define=HAVE_ECC --define=HAVE_POLY1305 --display_error_number --diag_warning=225 --diag_wrap=off --gen_func_subsections=on --preproc_with_compile --preproc_dependency="wolfssl/ctaocrypt/src/tfm.pp" --obj_directory="wolfssl/ctaocrypt/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


