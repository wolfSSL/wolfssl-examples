add_executable(Wifi
    ../src/blink.c
    ../src/wifi.c
    ../src/wifi_main.c
)

target_compile_definitions(Wifi PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    PICO_CYW43_ARCH_POLL
    NO_SYS=1
)

target_link_libraries(Wifi
    pico_stdlib
    pico_rand
    pico_lwip
    pico_cyw43_arch
    pico_lwip_nosys
    pico_async_context_poll
)

if (USE_UART)
    pico_enable_stdio_usb(Wifi 0)
    pico_enable_stdio_uart(Wifi 1)
else()
    pico_enable_stdio_usb(Wifi 1)
    pico_enable_stdio_uart(Wifi 0)
endif()

pico_add_extra_outputs(Wifi)
