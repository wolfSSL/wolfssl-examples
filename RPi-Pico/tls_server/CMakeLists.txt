add_executable(tls_Server
    ../src/wifi.c
    ../src/tcp.c
    ../src/tls.c
    ../src/time.c
    ../src/freertos_cb.c
    ../src/tlsServer_main.c
)

target_compile_definitions(tls_Server PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    NO_SYS=0
    LWIP_SOCKET=1
    LWIP_COMPAT_SOCKETS=1
    PING_USE_SOCKETS=1
    DATETIME=\"${DATETIME}\"
    WOLFSSL_USER_SETTINGS
)

target_link_libraries(tls_Server
    pico_stdlib
    pico_rand
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    wolfssl
)

pico_enable_stdio_usb(tls_Server 1)
pico_enable_stdio_uart(tls_Server 1)

pico_add_extra_outputs(tls_Server)