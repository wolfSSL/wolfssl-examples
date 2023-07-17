# CryptoCB Wisekey VaultIC Port

Support for the VaultIC 420:
 - Symmetric ciphers/modes: AES128-CBC
 - Hashes: SHA256
 - Asymmetric ciphers/functions: RSA-2048 Verify

This port requires the VaultIC 420 Dev Kit (including the vaultic_tls library) from Wisekey.  Please contact support@wolfssl.com for assistance.

## Building VaultIC Hardware Crypto Offload Support

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

To enable hardware offload support via cryptocb, configure wolfSSL to include crypto callbacks and to cross-compile a static library:

```
git clone git@github.com:wolfssl/wolfssl.git
cd wolfssl
./configure --host aarch64-linux-android CC=$NDK_CC AS=$NDK_AS RANLIB=$NDK_RANLIB AR=$NDK_AR LD=$NDK_LD STRIP=$NDK_STRIP CXX=$NDK_CXX  --enable-cryptocb --enable-static --disable-shared
make 
```

The port's `Makefile` also relies on certain source and header files within wolfSSL to be available.  The relative path to the wolfSSL directory is set by the `WOLFSSL_DIR` variable.  The wolfSSL source was updated to allow the test and benchmark programs to accept a compile-time CryptoCB DevId using the `FORCE_DEVID` macro, which is in a pending PR.

### Building VaultIC TLS Library

Please contact support@wolfssl.com for access to the modified VaultIC library files used for this port.

This port has modified the Wisekey provided vaultic_tls library to attempt to cancel authentication during startup in case a previous session ended without closing the VaultIC library by invoking `VltAuthClose()` immediately after opening the VaultIC interface.  The library is statically configured with user authentication hard-coded within the library itself and to use the Linux SPI Dev interface for communications.  Note that typical Linux permissions require root privileges to access the SPI device. 

For this example, the VaultIC library needed to be configured and cross-compiled to the Android target which is similar to the vendor-provided Raspberry Pi target.  Modifications we made to disable any GPIO's used to powerup and reset the chip as well as to set the cross compiler to use the Android NDK as above.  To build the VaultIC API and TLS libraries, follow the instructions provided by Wisekey, which are summarized as:

```
cd VaultIC-TLS_420/vaultic_tls-4xx/lib/VaultIC_420_TLS_Lib/target/raspberry/build
./build-all_spi.sh
```

The port's `Makefile` expects headers and corresponding functions from `vaultic_tls.h` and from the common VaultIC folders.  The relative base folder is set as the variable `VAULTIC_DIR`. 

### Building the Port's Benchmark and Test programs

This port leverages wolfCrypt's included benchmark and test programs as an example of how to statically link against wolfSSL and the VaultIC libraries as well as use the hardware offload abstractions provided within wolfSSL.  In this case, the wolfSSL software will attempt every cryptographic operating using the Wisekey VaultIC port via the CryptoCB method, but only the implemented functions will use the VaultIC hardware.  Every unsupported function will fallback and use the ARMv8-accelerated software implementation.

To build using the NDK cross-compiler configuration above:

```
make clean
make CC=$NDK_CC AS=$NDK_AS RANLIB=$NDK_RANLIB AR=$NDK_AR LD=$NDK_LD
```

The benchmark and test programs are built as part of the `make all` target and can be run on the target hardware as `wolfcrypt-bench` and `wolfcrypt-test` respectively.  Note that the test program expects the test certificates to be available on the target hardware.

## Benchmarks and Known Answer Tests

Benchmark using unoptimized ARMv8 (imx8) and VaultIC 420 for AES128-CBC:

```
Math: 	Multi-Precision: Wolf(SP) word-size=64 bits=4096 sp_int.c
wolfCrypt Benchmark (block bytes 1024, min 1.0 sec each)
Math: 	Multi-Precision: Wolf(SP) word-size=64 bits=4096 sp_int.c
wolfCrypt Benchmark (block bytes 1024, min 1.0 sec each)
AESCBC Internal with useDevice:0 and devid:56490420
AES-128-CBC-enc             78 MiB took 1.000 seconds,   77.604 MiB/s
AES-128-CBC-dec             77 MiB took 1.000 seconds,   77.361 MiB/s
AESCBC Internal with useDevice:1 and devid:56490420
AES-128-CBC-enc             25 KiB took 25.530 seconds,    0.979 KiB/s
AES-128-CBC-dec             25 KiB took 25.582 seconds,    0.977 KiB/s
```

Known answer test program output with VaultIC performing AES128-CBC test:

```
------------------------------------------------------------------------------
 wolfSSL version 5.6.3 with DevID:56490420
------------------------------------------------------------------------------
error    test passed!
MEMORY   test passed!
base64   test passed!
asn      test passed!
RANDOM   test passed!
MD5      test passed!
SHA      test passed!
SHA-224  test passed!
SHA-256  test passed!
SHA-384  test passed!
SHA-512  test passed!
SHA-512/224  test passed!
SHA-512/256  test passed!
SHA-3    test passed!
Hash     test passed!
HMAC-MD5 test passed!
HMAC-SHA test passed!
HMAC-SHA224 test passed!
HMAC-SHA256 test passed!
HMAC-SHA384 test passed!
HMAC-SHA512 test passed!
HMAC-SHA3   test passed!
HMAC-KDF    test passed!
TLSv1.3 KDF test passed!
GMAC     test passed!
Chacha   test passed!
POLY1305 test passed!
ChaCha20-Poly1305 AEAD test passed!
AES      test passed!
AES192   test passed!
AES256   test passed!
AES-GCM  test passed!
RSA      test passed!
DH       test passed!
PWDBASED test passed!
ECC      test passed!
logging  test passed!
time test passed!
mutex    test passed!
memcb    test passed!
crypto callback test passed!
Test complete
```

## Support

Email us at [support@wolfssl.com](mailto:support@wolfssl.com).
