# wolfSSL DTLS1.3 Project

This is an example minimally viable wolfSSL template to get started with your own project.

### Prerequisites

It is assumed the [ESP-IDF environment](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) has been installed.

```
gcc -o client-dtls13 client-dtls13.c -L/mnt/c/workspace/wolfssl-gojimmypi/src/.libs -I/mnt/c/workspace/wolfssl-gojimmypi/ -I/mnt/c/workspace/wolfssl-gojimmypi/include -DWOLFSSL_TLS13 -DWOLFSSL_DTLS -DWOLFSSL_DTLS13 -DWOLFSSL_USER_SETTINGS -lwolfssl -ldl -lm
```

Connect with Linux command line example:

```bash
# assuming wolfssl is in /workspace/wolfssl-$USER
cd /mnt/c/workspace/wolfssl-$USER
./autogen.sh
./configure --enable-dtls --enable-dtls13 --enable-tls13
make
./examples/dtls13client/client 192.168.1.37
```

```
./configure --enable-dtls --enable-dtls13 --enable-tls13 --enable-sm3 --enable-sm4-gcm --enable-sm2
make
./examples/dtls13client/client 192.168.1.37
```

Testing TLS 1.3 SM:

```
./examples/server/server -v 4 -l TLS13-SM4-GCM-SM3 \
    -c ./certs/sm2/server-sm2.pem -k ./certs/sm2/server-sm2-priv.pem \
    -A ./certs/sm2/client-sm2.pem -V &
./examples/client/client -v 4 -l TLS13-SM4-GCM-SM3 \
    -c ./certs/sm2/client-sm2.pem -k ./certs/sm2/client-sm2-priv.pem \
    -A ./certs/sm2/root-sm2.pem -C
```


### Files Included

- [main.c](./main/main.c) with a simple call to an Espressif library (`ESP_LOGI`) and a call to a wolfSSL library (`esp_ShowExtendedSystemInfo`) . 

- See [components/wolfssl/include](./components/wolfssl/include/user_settings.h) directory to edit the wolfSSL `user_settings.h`.

- Edit [main/CMakeLists.txt](./main/CMakeLists.txt) to add/remove source files.

- The [components/wolfssl/CMakeLists.txt](./components/wolfssl/CMakeLists.txt) typically does not need to be changed.

- Optional [VisualGDB Project](./VisualGDB/wolfssl_template_IDF_v5.1_ESP32.vgdbproj) for Visual Studio using ESP32 and ESP-IDF v5.1.

- Edit the project [CMakeLists.txt](./CMakeLists.txt) to optionally point this project's wolfSSL component source code at a different directory:

```
set(WOLFSSL_ROOT "~/workspace/wolfssl-other-source")
```


## Getting Started:

Here's an example using the command-line [idf.py](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-py.html).

Edit your `WRK_IDF_PATH`to point to your ESP-IDF install directory.

```
cd /mnt/C/workspace/wolfssl-gojimmypi/IDE/Espressif/ESP-IDF5/examples/wolfssl_dtls13_server

WRK_IDF_PATH=/mnt/c/SysGCC/esp32/esp-idf/v5.1

echo "Run export.sh from ${WRK_IDF_PATH}"
. ${WRK_IDF_PATH}/export.sh

# build the example:
idf.py build

# flash the code onto the serial device at /dev/ttyS19
idf.py flash -p /dev/ttyS19 -b 115200

# build, flash, and view UART output with one command:
idf.py flash -p /dev/ttyS19 -b 115200 monitor

# erase 
idf.py erase-flash -p /dev/ttyS9 -b 115200

# save defaults
idf.py save-defconfig
```

Press `Ctrl+]` to exit `idf.py monitor`. See [additional monitor keyboard commands](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-monitor.html).

## Other Examples:

For examples, see:

- [TLS Client](../wolfssl_client/README.md)
- [TLS Server](../wolfssl_server/README.md)
- [Benchmark](../wolfssl_benchmark/README.md)
- [Test](../wolfssl_test/README.md)
- [wolfssl-examples](https://github.com/wolfSSL/wolfssl-examples/tree/master/ESP32)
- [wolfssh-examples](https://github.com/wolfSSL/wolfssh-examples/tree/main/Espressif)



