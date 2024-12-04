# wolfSSL STM32MP13 FreeRTOS LwIP client example

This is an example to run an SSL client connection on an STM32MP135F-DK board.
It is designed to run from the STM32CubeIDE. It is a based on a combination of
the ST32MP13 FreeRTOS LwIP example and wolfSSL's LwIP example. The former has
been modified slightly to make the Ethernet more stable and to increase memory,
the latter has been modified to work asynchronous.

It will obtain an IP address via DHCP, connect to a wolfSSL echoserver, send a
message and display the reply on the ST-Link UART.

It runs from the DDR RAM on the board in engineering mode, details on how to do
this are in this document.

## Setting up

### Hardware

The board itself has dip switches to set the boot mode. These should be set to
off-off-on-off to set the board into "engineering mode". The MPU's RAM can
then be flashed via the ST-Link.

The board needs both the ST-Link and the power connector at the opposite side of
the board to be connected. `ETH1` also needs to be wired to your network.

### Git submodule

To set things up, you first need to update the git submodules to get the
STM32MP13 HAL, FreeRTOS and LwIP middleware:

```sh
git submodule update --init --recursive
```

A couple of files currently need a minor patch in the HAL. Edit the files
`STM32CubeMP13/Drivers/STM32MP13xx_HAL_Driver/Inc/stm32mp13xx_hal_cryp.h`
and `STM32CubeMP13/Drivers/STM32MP13xx_HAL_Driver/Inc/stm32mp13xx_hal_hash.h`,
add the following to each:

```c
#include "stm32mp13xx_hal_mdma.h"
```

### wolfSSL source code

You will need wolfSSL git checkout from 2024-12-03 onwards, or a version higher
than 5.7.4.

The wolfSSL source code needs to be in the directory this file is in, it should
be a directory called `wolfssl`. So, when running `ls`, you should see:

```
Core  startup_stm32mp135fxx_ca7_freeRTOS.c  STM32CubeIDE  STM32CubeMP13  system_stm32mp13xx_A7_freeRTOS.c  wolfssl
```

Once the source code is in place run the following from the directory this file
is in:

```sh
cp wolfssl/wolfssl/options.h.in wolfssl/wolfssl/options.h
```

### STM32CubeIDE

Import the project from the `STM32CubeIDE` directory. In the debug
configurations menu, under startup, remove `monitor reset`.

### IP Address

The IP address you wish the client to connect to needs to be configured. This is
in `Core/Src/wolfssl-client-example.c` using the variables `DEST_IP_ADDR0`,
`DEST_IP_ADDR1`, `DEST_IP_ADDR2` and `DEST_IP_ADDR3`.

## Running the demo

### DDR init

The DDR RAM on the board needs to be initialized before the wolfSSL project can
be executed. This needs to happen every time the wolfSSL project is run.

The directory `STM32CubeMP13/Projects/STM32MP135C-DK/Examples/DDR/DDR_Init`
contains the project to open on STM32CubeIDE to do this.

Open this project in STM32CubeIDE and execute it. If successful, the blue LED
will flash. The board is now ready to execute the wolfSSL example. This process
will need to be done every time you wish to run the wolfSSL project.

### echoserver

The echoserver is a tool which is run on your computer, the STM32 will connect
to this, send a message to the PC and receive an echo of that message back.

On your computer, obtain a copy of the wolfSSL source code, edit
`examples/echoserver/echoserver.c` and add `#define USE_ANY_ADDR 1` near the
top.

You can now compile and execute `./echoserver`.

### Execute the project

Open a terminal, connect to `/dev/ttyACM0` (on Linux, the port will be different
on other systems) at 115,200 board rate.

Then execute the project on STM32CubeIDE. You should see the following:

```
wolfSSL LwIP RTOS Example started
  State: Looking for DHCP server ...
IP address assigned by a DHCP server: 192.168.7.214
Connecting
Connected
TLS handshake done!
Got message: TLS **TEST 1** 
Connection shutdown

```
