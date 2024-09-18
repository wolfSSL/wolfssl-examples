add_executable(tls_Client
    ../src/wifi.c
    ../src/tcp.c
    ../src/tls.c
    ../src/time.c
    ../src/freertos_cb.c
    ../src/tlsClient_main.c
)

target_compile_definitions(tls_Client PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    TCP_SERVER=\"${TCP_SERVER}\"
    NO_SYS=0
    LWIP_SOCKET=1
    LWIP_COMPAT_SOCKETS=1
    PING_USE_SOCKETS=1
    DATETIME=\"${DATETIME}\"
    WOLFSSL_USER_SETTINGS
)

target_link_libraries(tls_Client
    pico_stdlib
    pico_rand
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    wolfssl
)

pico_enable_stdio_usb(tls_Client 1)
pico_enable_stdio_uart(tls_Client 1)

pico_add_extra_outputs(tls_Client)
