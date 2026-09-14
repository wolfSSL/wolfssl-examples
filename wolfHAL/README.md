# wolfCrypt + wolfHAL Example

Runs the wolfCrypt test suite and benchmark on bare metal, with AES and the RNG
served by the board's hardware.

[wolfHAL](https://github.com/wolfSSL/wolfHAL) is a portable hardware abstraction
layer with no OS, toolchain or platform dependencies. It provides a common API
for accessing hardware functionality, with the platform-specific configuration
kept in board files.

wolfSSL reaches it through `wolfcrypt/src/port/wolfHAL`, which registers a
crypto callback at `wolfCrypt_Init()`. wolfCrypt then routes AES to the
accelerator and falls back to software for anything the board does not offload.

## Supported boards

| Board | Offloaded |
| --- | --- |
| `stm32wb55xx_nucleo` | AES-ECB/CBC/GCM/CCM on AES1, RNG |

Each lives in `boards/<name>/` and owns its clock, pin and device setup, its
toolchain, and its linker script.

## Building

Needs `arm-none-eabi-gcc` and a wolfSSL checkout beside this repository:

```sh
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
git submodule update --init wolfHAL       # the wolfHAL drivers
make                                      # or: make BOARD=<name>
make flash
```

`WOLFSSL_ROOT` defaults to `../../wolfssl` and `WHAL_DIR` to the submodule;
override either on the command line. wolfCrypt is compiled from source rather
than linked, so no installed `libwolfssl` is involved.

## Adding a board

Copy `boards/stm32wb55xx_nucleo/` and adjust `wolfHAL_board.h` (device
initializers plus the `WC_WOLFHAL_*_DEV` macros naming what to offload),
`wolfHAL_board.c` (bring-up), `board.mk` (toolchain, driver list), `linker.ld`
and `ivt.c`. Then add a build step for it in `.github/workflows/wolfhal.yml`.
