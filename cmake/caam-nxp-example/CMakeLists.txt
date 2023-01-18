set(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

set(CMAKE_STATIC_LIBRARY_PREFIX)
set(CMAKE_STATIC_LIBRARY_SUFFIX)

set(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
set(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

project(wolfssl-caam-example)

set(MCUX_SDK_PROJECT_NAME CSR_example_cm7.elf)

message("Example cmake project including wolfSSL for NXP CAAM")

# add global define to include user_settings.h
add_compile_definitions(WOLFSSL_USER_SETTINGS)
add_compile_definitions(CACHE_MODE_WRITE_THROUGH=1)
add_compile_definitions(CRYPTO_USE_DRIVER_CAAM)

set(BUILD_SHARED_LIBS OFF)
set(WOLFSSL_EXAMPLES OFF)
set(WOLFSSL_CRYPT_TESTS OFF)
set(WOLFSSL_CAAM ON)
set(WOLFSSL_USER_SETTINGS ON)
set(WOLFSSL_CONFIG_H OFF)

if (CONFIG_BIG_ENDIAN)
    set(CMAKE_C_BYTE_ORDER    BIG_ENDIAN)
    set(CMAKE_CXX_BYTE_OREDER BIG_ENDIAN)
else ()
    set(CMAKE_C_BYTE_ORDER    LITTLE_ENDIAN)
    set(CMAKE_CXX_BYTE_OREDER LITTLE_ENDIAN)
endif()

# include location to example user_settings.h
include_directories(wolfssl/IDE/MCUEXPRESSO/RT1170)

# key name ProjDirPath is used in the example flags.cmake to find .ld files
set(ProjDirPath
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(NXP_EXAMPE_FREERTOS
    ${CMAKE_SOURCE_DIR}/boards/evkmimxrt1170/rtos_examples/freertos_hello/cm7/
)
set(NXP_PROJECT_TEMPLATE
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176/project_template/
)


include(${NXP_EXAMPE_FREERTOS}/armgcc/flags.cmake)
include(${NXP_EXAMPE_FREERTOS}/armgcc/config.cmake)

include_directories(
    ${NXP_PROJECT_TEMPLATE}
    ${NXP_EXAMPE_FREERTOS}
    rtos/freertos/freertos_kernel/include
    rtos/freertos/freertos_kernel/portable/GCC/ARM_CM4F
    devices/MIMXRT1176
    devices/MIMXRT1176/drivers
    CMSIS/Core/Include
)


# add in our application and boad files
add_executable(${MCUX_SDK_PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/wolfssl/IDE/MCUEXPRESSO/RT1170/CSR_example_cm7/source/CSR_example.c

    "${NXP_PROJECT_TEMPLATE}/pin_mux.c"
    "${NXP_PROJECT_TEMPLATE}/pin_mux.h"
    "${NXP_PROJECT_TEMPLATE}/board.c"
    "${NXP_PROJECT_TEMPLATE}/board.h"
    "${NXP_PROJECT_TEMPLATE}/clock_config.c"
    "${NXP_PROJECT_TEMPLATE}/clock_config.h"
    "${NXP_PROJECT_TEMPLATE}/peripherals.c"
    "${NXP_PROJECT_TEMPLATE}/peripherals.h"

    "${NXP_EXAMPE_FREERTOS}/FreeRTOSConfig.h"
    "${NXP_EXAMPE_FREERTOS}/dcd.c"
    "${NXP_EXAMPE_FREERTOS}/dcd.h"
    "${NXP_EXAMPE_FREERTOS}/evkmimxrt1170_connect_cm4_cm7side.jlinkscript"
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${NXP_EXAMPE_FREERTOS}
)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176/drivers
    ${CMAKE_SOURCE_DIR}/rtos/freertos/freertos_kernel
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176/utilities
    ${CMAKE_SOURCE_DIR}/components/uart
    ${CMAKE_SOURCE_DIR}/components/serial_manager
    ${CMAKE_SOURCE_DIR}/components/lists
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176/xip
    ${CMAKE_SOURCE_DIR}/boards/evkmimxrt1170/xip
    ${CMAKE_SOURCE_DIR}/devices/MIMXRT1176/drivers/cm7
    ${CMAKE_SOURCE_DIR}/components/silicon_id
    ${CMAKE_SOURCE_DIR}/CMSIS/Core/Include
)

# include modules
include(component_lpuart_adapter_MIMXRT1176_cm7)
include(component_serial_manager_MIMXRT1176_cm7)
include(component_lists_MIMXRT1176_cm7)
include(component_serial_manager_uart_MIMXRT1176_cm7)
include(component_silicon_id_MIMXRT1176_cm7)
include(device_MIMXRT1176_CMSIS_MIMXRT1176_cm7)
include(device_MIMXRT1176_startup_MIMXRT1176_cm7)
include(device_MIMXRT1176_system_MIMXRT1176_cm7)
include(utility_debug_console_MIMXRT1176_cm7)
include(utility_assert_MIMXRT1176_cm7)
include(utilities_misc_utilities_MIMXRT1176_cm7)
include(driver_clock_MIMXRT1176_cm7)
include(driver_lpuart_MIMXRT1176_cm7)
include(driver_common_MIMXRT1176_cm7)
include(driver_iomuxc_MIMXRT1176_cm7)
include(driver_igpio_MIMXRT1176_cm7)
include(driver_xip_device_MIMXRT1176_cm7)
include(driver_xip_board_evkmimxrt1170_MIMXRT1176_cm7)
include(driver_pmu_1_MIMXRT1176_cm7)
include(driver_dcdc_soc_MIMXRT1176_cm7)
include(driver_cache_armv7_m7_MIMXRT1176_cm7)
include(driver_anatop_ai_MIMXRT1176_cm7)
include(CMSIS_Include_core_cm_MIMXRT1176_cm7)
include(middleware_freertos-kernel_MIMXRT1176_cm7)
include(middleware_freertos-kernel_extension_MIMXRT1176_cm7)
include(middleware_freertos-kernel_heap_4_MIMXRT1176_cm7)

# include CAAM module
include(driver_caam_MIMXRT1176_cm7)

# include module hp snvs for RTC
include(driver_snvs_hp_MIMXRT1176_cm7)

# pull in the wolfSSL CMakeLists.txt for building
add_subdirectory(wolfssl)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE m)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE c)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE gcc)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE nosys)
target_link_libraries(${MCUX_SDK_PROJECT_NAME} PRIVATE wolfssl)
TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

