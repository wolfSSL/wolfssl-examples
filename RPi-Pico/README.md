## Getting Started

Details of the Pi Pico support in wolfSSL can be found in the
`wolfcrypt/src/port/pi_pico/README.md`.

This demonstration compiles several different utilities, including the wolfCrypt
benchmark and test suite.

### Prerequisites

You of course need a Pi Pico based board. Any RP2040 / RP2350 based board should
work, as long as it has a USB port to upload firmware to.

You need to have the [Raspberry Pi Pico SDK GitHub repository](https://github.com/raspberrypi/pico-sdk)
somewhere on your system. You also need the ARM compiler and CMake installed,
in Debian / Ubuntu you can do this using:

```
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

If you wish to use RISC-V with the RP2350 (ARM mode is default), you will need a
`riscv32-unknown-elf-gcc` compiler. You can search for binaries for this, or
compile the [RISC-V GNU Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
in multilib mode. You will also need to add symlinks from `riscv64-unknown-*` to
`riscv32-unknown-elf-*` if you build the toolchain from source. This is because
the multilib mode compiler is both 32bit and 64bit.

### 1. Set an export to the wolfSSL source directory.

```
export WOLFSSL_ROOT=/path/to/wolfssl/source
```

### 2. Setup pico-sdk and set `PICO_SDK_PATH`

```
export PICO_SDK_PATH=/path/to/pico-sdk
```

### 3. Set `FREERTOS_KERNEL_PATH`
If you want to use TCP/TLS server or client, You also need to have the [FreeRTOS-Kernel GitHub repository](https://github.com/FreeRTOS/FreeRTOS-Kernel).
```
export FREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel
```

### 4. cmake and make

The following CMAKE options are available:

* `PICO_BOARD` - This should be set to `pico` for a Pi Pico, `pico_w` for a Pi Pico with WiFi or `pico2` for a Pi Pico 2. A full list of boards for this option can be found [here](https://github.com/raspberrypi/pico-sdk/tree/master/src/boards/include/boards), just ignore the `.h` at the end.
* `USE_WIFI` - Build the tests that use WiFi, only works when `PICO_BOARD` defined has a CYW43 WiFi chip.
* `USE_UART` - Output to UART instead of USB, for the Pi Debug Probe.
* `WIFI_SSID` - The SSID to connect to (if `USE_WIFI` is set).
* `WIFI_PASSWORD` - The password used for the WiFi network (if `USE_WIFI` is set).
* `TEST_TCP_SERVER_IP` - The test server to connect to for the TCP client test (if `USE_WIFI` is set).

To use the RP2350 in RISC-V mode, add `-DPICO_PLATFORM=rp2350-riscv`.

```
$ cd RPi-Pico
$ cmake -DPICO_BOARD=pico_w ..
$ make
```

The build produces the following UF2 images:

- testwolfcrypt.uf2

- benchmark.uf2

- Wifi.uf2

- tcp_Client.uf2

- tls_Client.uf2

- tcp_Server.uf2

- tls_Server.uf2

### 5. Upload to the Pico

Hold the boot button and plug the Pico into your computer, you can then
drag/drop a `.uf2` to the Pico. It will stop becoming a USB mass storage device
and run immediately once the upload is complete. Alternatively, you can use
[picotool](https://github.com/raspberrypi/picotool) to upload a file:

```
sudo picotool load benchmark.uf2
sudo picotool reboot
```

### 6. Serial output

If you have not set `USE_UART`, once rebooted the USB port will turn into an
"Abstract Control Module" serial port. On Linux this will likely be
`/dev/ttyACM0`, or a number higher than `0` if you already have one. On macOS
this will be something like `/dev/cu.usbmodemXXXX`. The baud rate of this port
is 115200.

In Linux, most repositories have `minicom`. Install this using your package
manager and run:

```
minicom -b 115200 -o -D /dev/ttyACM0
```

If you need to exit at any time, it is CTRL-A followed by CTRL-X.
