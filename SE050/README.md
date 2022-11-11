# wolfSSL NXP SE050 Examples

wolfSSL includes support for the NXP SE050 Plug & Trust Secure Element. For
details about the NXP SE050, see [NXP's SE050 page](https://www.nxp.com/products/security-and-authentication/authentication/edgelock-se050-plug-trust-secure-element-family-enhanced-iot-security-with-maximum-flexibility:SE050).

This directory contains wolfSSL examples for NXP SE050. The examples have been
written for and tested on a Raspberry Pi with NXP SE050 EdgeLock development
kit.

For complete details on wolfSSL's support for NXP SE050, see
[README_SE050.md](https://www.github.com/wolfssl/wolfssl/wolfcrypt/src/port/nxp/README_SE050.md).
That document also describes how to download, build, and compile the SE05x
Middleware. It should be followed first before moving on to installing and
running these examples.

## Installing Example Applications

These instructions assume that the SE050 middleware has already been installed
in Raspbian and is located at `/home/pi/se_mw/simw-top`.

Before building these demo, the "wolfssl" folder which contains these demos
should be copied into the SE05x middleware source tree at:

```
/home/pi/se_mw/simw-top/demos/wolfssl
```

This will mean the wolfSSL demo applications will be at:

```
/home/pi/se_mw/simw-top/demos/wolfssl/wolfcrypt_benchmark
/home/pi/se_mw/simw-top/demos/wolfssl/wolfcrypt_generate_csr
/home/pi/se_mw/simw-top/demos/wolfssl/wolfcrypt_key_cert_insert
/home/pi/se_mw/simw-top/demos/wolfssl/wolfcrypt_test
```

The 'wolfssl' demo directory needs to be tied into the CMake build system. Open
the following file:

```
/home/pi/se_mw/simw-top/demos/CMakeLists.txt
```

Add the following at the bottom of this file:

```
ADD_SUBDIRECTORY(wolfssl)
```

If the SE05x middleware has not yet been set up for compilation:

```
$ cd /home/pi/se_mw/simw-top/scripts
$ ./create_cmake_projects.py rpi
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ ccmake .
< adjust options to match SE050 dev kit >
< 'c', to configure >
< 'g', to generate >
$ cmake --build .
```

This will build the SE05x middleware along with demos, including the wolfCrypt
test application.

**IMPORTANT:** Note that the wolfCrypt test and benchmark application examples
included here need benchmark.c, benchmark.h, test.c, and test.h copied over
from a valid wolfSSL source tree. See notes in those example README.md files
for more information.

## NXP SE050 EdgeLock Configuration

wolfSSL most recently tested these examples on a Raspberry Pi with NXP
EdgeLock development kit. The following CMake options were used. This uses
OpenSSL as the HostCrypto Provider and enables SCP03 authentication.

![CMake Configuration](images/se05x_middleware_cmake_openssl_scp03.png)

## Example Applications

### wolfCrypt Test Application

This application will run the wolfCrypt test application, running cryptography
test vectors through all algorithms enabled in wolfSSL. For details, see
[README.md](./wolfssl/wolfcrypt_test/README.md).

### wolfCrypt Benchmark Application

This application will run the wolfCrypt benchmark application, benchmarking
all enabled algorithms in wolfSSL. For details, see
[README.md](./wolfssl/wolfcrypt_benchmark/README.md).

### wolfCrypt SE050 Key and Certificate Insertion and Use Example

This example inserts and uses certificates and keys into the SE050. It does an
ECDSA verify operation of an ASCII-encoded {R,S} signature, reads back out the
RSA and ECC certificates imported and verifies they match the original ones,
then erases keys and certificates that were imported into the SE050. For
details, see [README.md](./wolfssl/wolfcrypt_key_cert_insert/README.md).

### wolfCrypt CSR Generation Example

This example generates a Certificate Signing Request (CSR) using a key
generated and stored in the SE050. This examples requires that wolfSSL be
compiled with `--enable-certgen --enable-certreq`. For details, see
[README.md](./wolfssl/wolfcrypt_generate_csr/README.md).

## Support

For support questions and issues, please email wolfSSL at support@wolfssl.com.

