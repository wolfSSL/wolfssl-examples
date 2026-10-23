# wolfCrypt SHA-2 / SHA-3 CCS projects for the LAUNCHXL-F28P55X

Two Code Composer Studio (CCS) projects for the TI LAUNCHXL-F28P55X (TMS320F28P550SJ, C2000 C28x): one for SHA-2, one for SHA-3. Each project is one small application file plus wolfCrypt, imported from a `.projectspec` in the same way as the C2000Ware examples.

| Project | projectspec | Algorithms |
|---|---|---|
| `wolfssl_sha2` | `ccs/sha2/wolfssl_sha2.projectspec` | SHA-224, SHA-256, SHA-384, SHA-512, SHA-512/224, SHA-512/256 |
| `wolfssl_sha3` | `ccs/sha3/wolfssl_sha3.projectspec` | SHA3-224, SHA3-256, SHA3-384, SHA3-512, SHAKE128, SHAKE256 |

## Prerequisites

- Code Composer Studio 20.x. CCS 12.x imports the same `.projectspec` format but has not been tested.
- C2000Ware 26.01 (`C2000Ware_26_01_00_00`), installed where CCS can find it (CCS discovers it under `~/ti` or `C:\ti`).
- TI C2000 code generation tools 22.6.3.LTS. 25.11.0.LTS has also been tested. To use a different installed version, change it under Project Properties > General > Compiler version.
- A clone of the wolfSSL master branch. The C28x has 16-bit bytes (`CHAR_BIT == 16`) and needs the support for that in current wolfSSL.

The projects expect wolfSSL to be cloned next to wolfssl-examples:

```
<dir>/wolfssl            git clone https://github.com/wolfSSL/wolfssl.git
<dir>/wolfssl-examples   git clone https://github.com/wolfSSL/wolfssl-examples.git
```

## Import and build

1. File > Import Projects, and select `ccs/sha2/wolfssl_sha2.projectspec` or `ccs/sha3/wolfssl_sha3.projectspec`.
2. The application files (`test_sha2.c` or `test_sha3.c`, `c2000_board.c`, `c2000_board.h`, `user_settings.h`, `28p55x_wolf_flash_lnk.cmd`) and the C2000Ware device files are copied into the new project, so you can edit them freely. The wolfCrypt sources in the `wolfcrypt` folder are links into the wolfSSL clone.
3. Pick a build configuration and build:
   - `CPU1_LAUNCHXL_FLASH`: optimized (`-O2`).
   - `CPU1_LAUNCHXL_FLASH_DEBUG`: no optimization and full symbols, for stepping through the hash calls.

If wolfSSL is somewhere else, `WOLFSSL_ROOT` must be changed in two places in each project (the projectspec records it as both a linked-resource path and a build variable):

- Project Properties > Resource > Linked Resources > Path Variables: `WOLFSSL_ROOT`
- Project Properties > Build > Variables (select "All configurations"): `WOLFSSL_ROOT`

## Flash, run and read the output

1. Use the LaunchPad target configuration copied into the project (`targetConfigs/TMS320F28P550SJ9_LaunchPad.ccxml`) and start a debug session (Run > Debug), which programs flash.
2. Open a serial terminal on the LaunchPad's XDS110 "Application/User UART" port at 115200 8N1.
3. Run. The output is printed on the UART:

```
=== wolfCrypt SHA-2 example on TI C2000 LAUNCHXL-F28P55X ===

--- Known-answer tests ---
SHA-224      23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7
SHA-224      KAT: PASS
SHA-256      ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
SHA-256      KAT: PASS
...
--- Throughput (150 MHz) ---
SHA-256      64 KiB in 0.222 s = 288.0 KiB/s
...
SHA-2 example: PASS (0 failure(s))
=== done ===
```

The UART output stays clean only while the debugger is not halting the target, so let it run freely (or disconnect after flashing). To send `printf` to the CCS Console instead of the UART, remove `WOLF_C2000_SCI_STDOUT` under Project Properties > Build > C2000 Compiler > Predefined Symbols. The CCS Console is much slower.

## Code flow

`main()` in `test_sha2.c` / `test_sha3.c`:

1. `c2000_board_init()` (`c2000_board.c`): PLL to 150 MHz, GPIO, SCIA console redirected to `stdout`, and CPU Timer 0 as the time base for `current_time()`.
2. One function per algorithm, e.g. `sha256_example()`, showing the complete wolfCrypt call sequence on the FIPS 180-4 / FIPS 202 "abc" test vector: `wc_InitSha256()` -> `wc_Sha256Update()` -> `wc_Sha256Final()` -> `wc_Sha256Free()`. The digest is printed and compared to the expected value (`PASS` / `FAIL`).
3. A streaming example that feeds one message through several `Update` calls (`sha256_stream_example()` / `shake256_stream_example()`).
4. `hash_bench()`: hashes 64 KiB per algorithm through the generic `wc_HashInit()` / `wc_HashUpdate()` / `wc_HashFinal()` API and prints the throughput.

## Adding wolfCrypt hashing to your own CCS project

1. Clone wolfSSL master.
2. Add these files from `<wolfssl>/wolfcrypt/src/` to the project (Add Files, or link them):
   - Always: `error.c`, `wc_port.c`, `memory.c`, `logging.c`, `misc.c`, `hash.c`.
   - SHA-2: `sha256.c`, `sha512.c`. SHA-3: `sha3.c`.
3. Copy `ccs/sha2/user_settings.h` or `ccs/sha3/user_settings.h` into your project. The platform section must stay as is: it sets the C28x integer widths and the `CHAR_BIT == 16` handling. Then adjust the algorithm section.
4. Project Properties > Build > C2000 Compiler:
   - Include Options: add your project directory (for `user_settings.h`) and the wolfSSL root directory (the one containing the `wolfssl/` folder).
   - Predefined Symbols: add `WOLFSSL_USER_SETTINGS`.
5. Include `<wolfssl/wolfcrypt/settings.h>` before any other wolfCrypt header, then `<wolfssl/wolfcrypt/sha256.h>`, `sha512.h` or `sha3.h`.
6. Linker command file: these examples use `28p55x_wolf_flash_lnk.cmd`, which places a 16 KW stack in low RAM (the C28x stack must sit below 0x10000) and runs the SHA-3 core `BlockSha3` from RAM for speed. The hash examples make no heap allocations.

On the C28x a `byte` is 16 bits and holds one octet: every wolfCrypt byte buffer stores one octet per 16-bit cell, and lengths are octet counts. Data that arrives packed (two octets per cell) from flash or a peripheral must be unpacked with `wc_UnpackOctets()` before it is hashed.
