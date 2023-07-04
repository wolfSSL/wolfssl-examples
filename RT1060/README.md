The examples for NXP i.MXRT1060 support the MMIMXRT1060-EVKB and the MMIMXRT1060-EVK boards.

Building and running the examples
====

Set up the NXP MCUXpresso SDK
----

- Use the NXP _MCUXpresso SDK Builder_ available from NXP (https://mcuxpresso.nxp.com) to generate the SDK choosing:

    - _Required Toolchains_: `GCC ARM Embedded`

    For **MIMXRT1060-EVKB**:
    - _Development Board_: `MIMXRT1060-EVKB (IMXRT1062xxxxA)`
    - _Version_: `2.13.1`

    For **EVK-IMXRT1060**:
    - _Development Board_: `EVK-MIMXRT1060 (IMXRT1062xxxxA)`
    - _Version_: `2.8.2`

- Download the generated SDK archive and unpack it into:

    For **MIMXRT1060-EVKB**:
    - `wolfssl-examples/RT1060/SDK_2_13_1_MIMXRT1060-EVKB`

    For **EVK-MIMXRT1060**:
    - `wolfssl-examples/RT1060/SDK_2_8_2_EVK-MIMXRT1060`

Set up ARM cross-compilation tools
----

- Ensure that a recent arm-gcc toolchain is installed and reachable via the `CC` environment variable.

    - A recent toolchain can be obtained from ARM (https://developer.arm.com)

    - Example: `CC=arm-none-eabi-gcc`

Set up wolfSSL
----

- In the same directory as `wolfssl-examples` (i.e. the parent directory):

```
$ git clone git@github.com:wolfssl/wolfssl.git
```

Build wolfSSL
----

- In the `wolfssl-examples/RT1060` directory:

    - For **MIMXRT1060-EVKB**:

```
$ SDK=SDK_2_13_1_MIMXRT1060-EVKB make
```

- For **EVK-MIMXRT106**:


```
$ SDK=SDK_2_8_2_EVK-MIMXRT1060 make
```

The resulting binary files will be in `wolfssl-examples/RT1060`:

- `wolfcrypt-test.bin` - wolfCrypt crytography test

- `wolfcrypt-benchmark.bin` - wolfCrypt cryptography benchmark

Download and run the built wolfSSL examples
----

- Connect the board to the host computer via the debug port

- Copy a wolfSSL example binary file to the storage device associated with the board

- Wait for the board to flash the image and reset

- Open a terminal console emulator and connect it to the port associated with the board, e.g. `/dev/ttyACM0`

- Reset the board.  The wolfSSL example output will display in the terminal console

Notes
====

- EVK-MIMXRT1060 benchmark results are also available in the file [results.md](results.md)

- MIMXRT1060-EVKB example developed using _arm-none-eabi-gcc (15:10.3-2021.07-4) 10.3.1 20210621 (release)_ on _Ubuntu 22.04LTS_ host
