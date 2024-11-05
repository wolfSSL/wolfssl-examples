## Getting Started

This example includes wolfSSL test, benchmark, Wifi, TCP/TLS client.


### 1. Download files

```
$ git clone https://github.com/raspberrypi/pico-sdk
$ git clone https://github.com/raspberrypi/pico-examples
$ git clone hhttps://github.com/FreeRTOS
$ git clone https://github.com/wolfssl/wolfssl
$ git clone https://github.com/wolfssl-jp/RPi-pico-w
```


### 2. Define path

```
$ export PICO_SDK_PATH=/your/pico-sdk/path
$ export PICO_EXAMPLES_PATH=/your/pico-examples/path
$ export FREERTOS_KERNEL_PATH=/your/FreeRTOS-kernel/path
$ export WOLFSSL_ROOT=/your/wolfssl-root/path
```

### 3. cmake and make

```
$ cd wolfssl-examples/RPi-Pico
$ cmake -DPICO_BOARD=pico_w -DWIFI_SSID=your-wifi-ssid\
    -DWIFI_PASSWORD=your-wifi-password\
    -DTCP_SERVER=ip-addr\
    -DDATETIME=yymmddhhmmss .
$ make
```

DATETIME is for certificate validity check. Not need to be very acurrate.

### 4. Target files

- testwolfcrypt.uf2
- benchmark.uf2
- tcp_Client.uf2
- tls_Client.uf2
- tcp_Server.uf2
- tls_Server.uf2

Console output is to USB serial


### 5. Test server

$ git clone https://github.com/wolfssl/wolfssl-examples
$ cd wolfssl-examples/tls
$ make

TCP Server:
$ ./server-tcp

TLS Server:
$ ./server-tls

TCP Client:
$ ./client-tcp xxx.xxx.xxx.xxx

TLS Client:
$ ./client-tls xxx.xxx.xxx.xxx



### References

- Raspberry Pi Pico and Pico W<br>
https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html

- Connecting to the Internet with Raspberry Pi Pico W<br>
https://datasheets.raspberrypi.com/picow/connecting-to-the-internet-with-pico-w.pdf
