# Examples using wolfSSL with Platform Security Architecture (PSA)
==================================================================

## Example lists

    - client/server TLS1.3 ECC example
    - Trusted Firmware-M TLS1.3 on Nucleo-l552ZE-Q board

## client/server TLS1.3 ECDH-ECC example

These examples are modified versions of client/server examples found in tls/ but
using PSA for ECDH, ECDSA, HKDF, AES-GCM and SHA256.

### BUILD

To build the client/server examples you must provide the path to a static
library that implements the PSA interface using the environment variable
`PSA_LIB_PATH`. You can optionally specify the location of the PSA headers with
`PSA_INCLUDE`. After that you can run `make` to build the `client-tls13-ecc-psa`
and `server-tls13-ecc-psa`.

```
PSA_INCLUDE=/path/to/psa/include PSA_LIB_PATH=/path/to/psa/library.a make
```

wolfSSL must be built with PSA (`--enable-psa`) and public key callbacks
(`--enable-pkcallbacks`) support and must be built using the same PSA headers
used to compile the examples.

### Using MbedTLS PSA implementation

You can test these examples with mbedTLS PSA implementation. For this task the
helper script `build_with_mbedtls_psa.sh` is provided. It must run from the
wolfSSL source root directory and it compiles the mbedTLS library in
`/tmp/mbedtls` . To use the script and then compile the examples use these
commands:

```
cd /path/to/wolfSSL/src;
./path/to/wolfssl-examples/psa/build_with_mbedtls_psa.sh
make install
cd /path/to/wolfssl-examples/psa/
export PSA_INCLUDE=/tmp/mbedtls/build/include
export PSA_LIB_PATH=/tmp/mbedtls/build/library/libmbedcrypto.a 
make
```

## Trusted Firmware-M TLS1.3

TLS1.3 client/server exchange a small message over memory in PSA enabled
Trusted Firmware-M (TF-M) on Nucleo-l552ZE-Q board.

This example is provided as a patch to the TF-M test repo, which is normally
used as the default Non Secure app in the TF-M repo.  This way the example
integrates smoothly inside the TF-M build system.

The general requirements to build TF-M are listed here
[TF-M doc](https://tf-m-user-guide.trustedfirmware.org/docs/getting_started/tfm_getting_started.html)

To compile TF-M on Nucleo-l552ZE-Q board you additionally need:
    - GNU Arm compiler v7.3.1+ [toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
    - STM32_Programmer_CLI see [here](https://www.st.com/en/development-tools/stm32cubeprog.html)

To have all the needed binary artifacts to flash on the board you need three
interacting parts: the main TF-M repo (for bootloader and Secure world), wolfSSL
PSA-enabled library, and the modified TF-M test repo (for Non-Secure world). The
provided script `build_tfm_example.sh` automatically downloads and compile all
the needed components and produces the final build artifacts. The toolchain
needs to be available on the default path for the script to work.

CAVEATS:
The example only works with TF-M commit ID f07cc31545bbba3bad1806ed078c3aee3a09dc52

After running `build_tfm_example.sh` you can flash the binaries artifacts from
the TF-M build directory (defaults to `/tmp/wolfssl_tfm/tfm/build`) and run:

```
./regression.sh && ./TFM_UPDATE.sh
```

to flash on the nucelo board. Remember that this step needs
`STM32_Programmer_CLI`installed and on the default PATH.

After that you will see client and server interacting on the UART of the board:

```
[Sec Thread] Secure image initializing!
TF-M FP mode: Software
Booting TFM v1.5.0
Non-Secure system starting...
wolfSSL demo
wolfSSL_Init Success
wolfSSL provisioning server secret key
Server is starting
Client is starting
Overriding cert date error as example for bad clock testing
Received message from client:
hello wolfssl!
```
