Building wolfCrypt benchmark on i.MX RT1060-EVK

1. Go to  https://mcuxpresso.nxp.com and download the SDK archive for RT1060 - version 2.8.2

2. Unpack the SDK archive into `./SDK-2.8.2_EVK-MIMXRT1060`

3. Download latest wolfSSL (e.g. clone via `git clone git@github.com:wolfssl/wolfssl.git`) in the same directory as `wolfssl-examples`

4. Ensure that a recent arm-gcc toolchain is installed and reachable from your path. A recent toolchain can be obtained from:
    https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

5. Clone this repository

6. Enter the repository directory and run `make`

7. Connect the RT1060-EVK to the computer via the OpenSDA USB port (L23)

8. Copy either `wolfcrypt-test.bin` or `wolfcrypt-benchmark.bin` to the storage device associated to the RT1060-EVK

9. Open a terminal console emulator and connect it to the port associated to the RT1060-RVK (e.g. `/dev/ttyACM0`)

10. Reset the board to start the test or benchmark and read the result on the terminal console


Benchmark results are also available in the file [results.md](results.md)

