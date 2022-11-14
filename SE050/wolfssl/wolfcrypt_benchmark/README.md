# wolfCrypt Benchmark Demo Application

This demo application runs the wolfCrypt benchmark app. It benchmarks all
algorithms enabled in wolfCrypt. It has been tested using a Raspberry Pi
and SE050 EdgeLock development kit.

## Building the Demo

Before building this demo, follow initial setup instructions in the parent
[README.md](../../README.md).

**IMPORTANT:** This example requires the benchmark.c and benchmark.h files from
a wolfSSL source package be copied into this directory before compiling and
running the example. There are stub files included in this example directory
that should be overwritten:

```
$ cp wolfssl-X.X.X/wolfcrypt/benchmark/benchmark.c ./
$ cp wolfssl-X.X.X/wolfcrypt/benchmark/benchmark.h ./
```

Once this example directory has been copied into the SE05x middleware directory
tree in the correct location, compiling the middleware will also compile this
demo application:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ cmake --build .
```

## Running the Demo

To run the demo:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c/bin
$ ./wolfcrypt_benchmark
```

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfcrypt_benchmark
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
sss   :WARN :Communication channel is Plain.
sss   :WARN :!!!Not recommended for production use.!!!
App   :INFO :running setconfig
App   :INFO :Ran setconfig successfully
wolfCrypt Benchmark (block bytes 1024, min 1.0 sec each)
RNG                         12 MiB took 1.001 seconds,   11.928 MiB/s
AES-128-CBC-enc             20 MiB took 1.000 seconds,   19.724 MiB/s
AES-128-CBC-dec             19 MiB took 1.001 seconds,   19.438 MiB/s
AES-192-CBC-enc             17 MiB took 1.001 seconds,   16.982 MiB/s
AES-192-CBC-dec             17 MiB took 1.001 seconds,   16.612 MiB/s
AES-256-CBC-enc             15 MiB took 1.001 seconds,   14.755 MiB/s
AES-256-CBC-dec             15 MiB took 1.001 seconds,   14.580 MiB/s
AES-128-GCM-enc             10 MiB took 1.002 seconds,    9.967 MiB/s
AES-128-GCM-dec             10 MiB took 1.002 seconds,    9.965 MiB/s
AES-192-GCM-enc              9 MiB took 1.002 seconds,    9.208 MiB/s
AES-192-GCM-dec              9 MiB took 1.000 seconds,    9.202 MiB/s
AES-256-GCM-enc              8 MiB took 1.001 seconds,    8.492 MiB/s
AES-256-GCM-dec              8 MiB took 1.000 seconds,    8.495 MiB/s
AES-128-GCM-enc-no_AAD      10 MiB took 1.001 seconds,    9.975 MiB/s
AES-128-GCM-dec-no_AAD      10 MiB took 1.002 seconds,   10.035 MiB/s
AES-192-GCM-enc-no_AAD       9 MiB took 1.001 seconds,    9.269 MiB/s
AES-192-GCM-dec-no_AAD       9 MiB took 1.002 seconds,    9.262 MiB/s
AES-256-GCM-enc-no_AAD       9 MiB took 1.002 seconds,    8.554 MiB/s
AES-256-GCM-dec-no_AAD       9 MiB took 1.003 seconds,    8.545 MiB/s
GMAC Table 4-bit            22 MiB took 1.000 seconds,   21.581 MiB/s
AES-128-ECB-enc             15 MiB took 1.000 seconds,   14.996 MiB/s
AES-128-ECB-dec             15 MiB took 1.000 seconds,   14.983 MiB/s
AES-192-ECB-enc             13 MiB took 1.000 seconds,   13.356 MiB/s
AES-192-ECB-dec             13 MiB took 1.000 seconds,   13.240 MiB/s
AES-256-ECB-enc             12 MiB took 1.000 seconds,   11.929 MiB/s
AES-256-ECB-dec             12 MiB took 1.000 seconds,   11.921 MiB/s
CHACHA                      38 MiB took 1.000 seconds,   37.518 MiB/s
CHA-POLY                    26 MiB took 1.000 seconds,   26.211 MiB/s
MD5                        129 MiB took 1.000 seconds,  129.068 MiB/s
POLY1305                   118 MiB took 1.000 seconds,  118.446 MiB/s
SHA                         66 MiB took 1.000 seconds,   66.223 MiB/s
SHA-256                     31 MiB took 1.001 seconds,   31.328 MiB/s
SHA-384                     16 MiB took 1.000 seconds,   15.918 MiB/s
SHA-512                     16 MiB took 1.000 seconds,   15.914 MiB/s
HMAC-MD5                   127 MiB took 1.000 seconds,  126.965 MiB/s
HMAC-SHA                    65 MiB took 1.000 seconds,   65.453 MiB/s
HMAC-SHA256                 31 MiB took 1.001 seconds,   30.983 MiB/s
HMAC-SHA384                 16 MiB took 1.000 seconds,   15.551 MiB/s
HMAC-SHA512                 16 MiB took 1.001 seconds,   15.653 MiB/s
PBKDF2                       4 KiB took 1.002 seconds,    3.618 KiB/s
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id FFFF
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10000
RSA     1024 key gen         2 ops took 1.377 sec, avg 688.364 ms, 1.453 ops/sec
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10001
RSA     2048 key gen         1 ops took 7.441 sec, avg 7440.538 ms, 0.134 ops/sec
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10002
RSA     2048 public         10 ops took 1.133 sec, avg 113.271 ms, 8.828 ops/sec
RSA     2048 private         6 ops took 1.278 sec, avg 212.942 ms, 4.696 ops/sec
DH      2048 key gen        46 ops took 1.002 sec, avg 21.780 ms, 45.915 ops/sec
DH      2048 agree          22 ops took 1.027 sec, avg 46.693 ms, 21.416 ops/sec
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10003
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10004
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10005
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10006
ECC   [      SECP256R1]   256 key gen         4 ops took 1.149 sec, avg 287.259 ms, 3.481 ops/sec
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10007
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10008
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10009
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000A
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000B
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000C
ECDHE [      SECP256R1]   256 agree           4 ops took 1.710 sec, avg 427.612 ms, 2.339 ops/sec
ECDSA [      SECP256R1]   256 sign           10 ops took 1.010 sec, avg 100.953 ms, 9.906 ops/sec
ECDSA [      SECP256R1]   256 verify         10 ops took 1.010 sec, avg 100.998 ms, 9.901 ops/sec
Benchmark complete
```

## Demo Notes

The `benchmark.c` and `benchmark.h` file in this directory have been copied
directly from the wolfSSL download directory, at:

```
wolfssl-X.X.X/wolfcrypt/benchmark/benchmark.c
wolfssl-X.X.X/wolfcrypt/benchmark/benchmark.h
```

