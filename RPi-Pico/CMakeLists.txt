cmake_minimum_required(VERSION 3.13)

# Pull in Pico and FreeRTOS
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
#include(pico_extras_import_optional.cmake)
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)


if(PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0(or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wolf_pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

option(USE_UART "Output over UART instead of USB" OFF)
option(USE_WIFI "Enable WiFi" OFF)
option(WIFI_SSID "The WiFi SSID to connect to" "")
option(WIFI_PASSWORD "The WiFi password" "")
option(TEST_TCP_SERVER_IP "The TCP test server IP" "")

if (USE_WIFI AND NOT PICO_CYW43_SUPPORTED)
    message(FATAL_ERROR "You can only set USE_WIFI when a PICO_BOARD with wifi is used")
endif()

### Global Include Path
    include_directories(config)
    include_directories(include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include)
    include_directories(${PICO_SDK_PATH}/lib/lwip/src/include)
    include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_async_context/include)
    #include_directories(${PICO_SDK_PATH}/lib/btstack/test/embedded/)

    set(WOLFSSL_ROOT $ENV{WOLFSSL_ROOT})
    set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
    include_directories(${FREERTOS_KERNEL_PATH}/include/)
    include_directories(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/include)
    include_directories(${WOLFSSL_ROOT})
### End of Global Include Path

    include(wolfssl_import.cmake)

    add_subdirectory(testwolfcrypt)
    add_subdirectory(benchmark)
    if (USE_WIFI)
        add_subdirectory(wifi)
        add_subdirectory(tcp_client)
        add_subdirectory(tcp_server)
        add_subdirectory(tls_client)
        add_subdirectory(tls_server)
    endif()
