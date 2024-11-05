cmake_minimum_required(VERSION 3.13)

# Pull in Pico, Pico-examples and FreeRTOS
# Pico Examples is only for referring ico_sdk_import.cmake and pico_extras_import_optional.cmake
set(PICO_BOARD pico_w)
include($ENV{PICO_EXAMPLES_PATH}/pico_sdk_import.cmake)
include($ENV{PICO_EXAMPLES_PATH}/pico_extras_import_optional.cmake)
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
set(FREERTOS_KERNEL_PATH $ENV{FREERTOS-KERNEL_PATH})

if(PICO_SDK_VERSION_STRING VERSION_LESS " 1.3.0 ")
    message(FATAL_ERROR " Raspberry Pi Pico SDK version 1.3.0(or later) required. Your version is ${PICO_SDK_VERSION_STRING} ")
endif()

project(wolf_pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

### Global Include Path
    include_directories(config)
    include_directories(include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/src/include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include)
    #include_directories(${PICO_SDK_PATH}/lib/btstack/test/embedded/)
    include_directories(${FREERTOS_KERNEL_PATH}/include/)
    include_directories(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include)
    include_directories(${WOLFSSL_ROOT})
###  End of Global Include Path
    
    include(wolfssl_import.cmake)

    add_subdirectory(testwolfcrypt)
    add_subdirectory(benchmark)
    add_subdirectory(tcp_client)
    add_subdirectory(tcp_server)
    add_subdirectory(tls_client)
    add_subdirectory(tls_server)

