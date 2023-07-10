# Wisekey VaultIC Port

Support for the VaultIC 420:
 - Symmetric ciphers/modes: AES128-CBC
 - Hashes: SHA256
 - Asymmetric ciphers/functions: RSA-2048 Verify

This port requires the VaultIC 420 Dev Kit (including the vaultic_tls library) from Wisekey.  Please contact support@wolfssl.com for assistance.

## VaultIC Hardware Crypto Offload

This port is setup for cross compiling to an ARMv8 Android system and the build configuration assume the Android NDK is installed.  Environment variables can be setup, for example:

```
export NDK=~/Library/Android/sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/darwin-x86_64
export PATH=$PATH:~/Documents/Masters/android/platform-tools
export NDK_AR=$NDK/bin/llvm-ar
export NDK_CC=$NDK/bin/aarch64-linux-android30-clang
export NDK_AS=$NDK_CC
export NDK_CXX=$NDK/bin/aarch64-linux-android30-clang++
export NDK_LD=$NDK/bin/ld
export NDK_RANLIB=$NDK/bin/llvm-ranlib
export NDK_STRIP=$NDK/bin/llvm-strip
```

### Building wolfSSL

To enable hardware offload support, configure wolfSSL to include crypto callbacks and to cross-compile a static library:

```
./configure --host aarch64-linux-android CC=$NDK_CC AS=$NDK_AS RANLIB=$NDK_RANLIB AR=$NDK_AR LD=$NDK_LD STRIP=$NDK_STRIP CXX=$NDK_CXX  --enable-cryptocb --enable-static --disable-shared --enable-armasm
make 
```

The port's `Makefile` also relies on certain source and header files within wolfSSL to be available.  The relative path to the wolfSSL directory is set by the `WOLFSSL` variable.  The wolfSSL source was updated to allow the test and benchmark programs to accept a compile-time CryptoCB DevId using the `FORCE_DEVID` macro, which is in a pending PR.

### Building VaultIC TLS Library

This port has modified the Wisekey provided vaultic_tls library to attempt to cancel authentication during startup in case a previous session ended without closing the VaultIC library by invoking `VltCancelAuthentication()` immediately after opening the VaultIC interface.  The library is statically configured with user authentication hard-coded within the library itself and to use the Linux SPI Dev interface for communications.  Note that typical Linux permissions require root privileges to access the SPI device. 

For this example, the VaultIC library needed to be configured and cross-compiled to the Android target which is similar to the vendor-provided Raspberry Pi target.  Modifications we made to disable any GPIO's used to powerup and reset the chip as well as to set the cross compiler to use the Android NDK as above.  To build the VaultIC API and TLS libraries, follow the instructions provided by Wisekey, which are summarized as:

```
cd VaultIC-TLS_420/vaultic_tls-4xx/lib/VaultIC_420_TLS_Lib/target/raspberry/build
./build-all_spi.sh
```

The port's `Makefile` expects headers and corresponding functions from `vaultic_tls.h` and from the common VaultIC folders.  The relative base folder is set as the variable `VAULTIC`. 

### Building the Port's Benchmark and Test programs

This port leverages wolfCrypt's included benchmark and test programs as an example of how to statically link against wolfSSL and the VaultIC libraries as well as use the hardware offload abstractions provided within wolfSSL.  In this case, the wolfSSL software will attempt every cryptographic operating using the Wisekey VaultIC port via the CryptoCB method, but only the implemented functions will use the VaultIC hardware.  Every unsupported function will fallback and use the ARMv8-accelerated software implementation.

The benchmark and test programs are built as part of the `make all` target and can be run on the target hardware as `wolfcrypt-bench` and `wolfcrypt-test` respectively.  Note that the test program expects the test certificates to be available on the target hardware.

### Benchmarks and Memory Use

Software only implementation (STM32L4 120Mhz, Cortex-M4, Fast Math):

```
ECDHE    256 key gen       SW    4 ops took 1.278 sec, avg 319.500 ms,  3.130 ops/sec
ECDHE    256 agree         SW    4 ops took 1.306 sec, avg 326.500 ms,  3.063 ops/sec
ECDSA    256 sign          SW    4 ops took 1.298 sec, avg 324.500 ms,  3.082 ops/sec
ECDSA    256 verify        SW    2 ops took 1.283 sec, avg 641.500 ms,  1.559 ops/sec
```

Memory Use:

```
Peak Stack: 18456
Peak Heap: 2640
Total: 21096
```


STSAFE-A100 acceleration:

```
ECDHE    256 key gen       HW    8 ops took 1.008 sec, avg 126.000 ms,  7.937 ops/sec
ECDHE    256 agree         HW    6 ops took 1.051 sec, avg 175.167 ms,  5.709 ops/sec
ECDSA    256 sign          HW   14 ops took 1.161 sec, avg  82.929 ms, 12.059 ops/sec
ECDSA    256 verify        HW    8 ops took 1.184 sec, avg 148.000 ms,  6.757 ops/sec
```

Memory Use:

```
Peak Stack: 9592
Peak Heap: 170
Total: 9762
```


## Support

Email us at [support@wolfssl.com](mailto:support@wolfssl.com).
