add_executable(tcp_Client
    ../src/blink.c
    ../src/wifi.c
    ../src/tcp.c
    ../src/tcpClient_main.c
    ../src/idleMemory.c
)

target_compile_definitions(tcp_Client PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    TCP_SERVER=\"${TCP_SERVER}\"
    NO_SYS=0
    LWIP_SOCKET
    LWIP_COMPAT_SOCKETS
    PING_USE_SOCKETS=1
)

target_link_libraries(tcp_Client
    pico_stdlib
    pico_rand
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
)

pico_enable_stdio_usb(tcp_Client 1)
pico_enable_stdio_uart(tcp_Client 1)

pico_add_extra_outputs(tcp_Client)