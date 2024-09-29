add_executable(tcp_Server
    ../src/wifi.c
    ../src/tcp.c
    ../src/tcpServer_main.c
    ../src/freertos_cb.c
    )

target_compile_definitions(tcp_Server PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    NO_SYS=0
    LWIP_SOCKET
    LWIP_COMPAT_SOCKETS
    PING_USE_SOCKETS=1
)

target_link_libraries(tcp_Server
    pico_stdlib
    pico_rand
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(tcp_Server 1)
pico_enable_stdio_uart(tcp_Server 1)

pico_add_extra_outputs(tcp_Server)
