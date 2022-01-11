# Examples using wolfSSL with Platform Security Architecture (PSA)
==================================================================

## Example lists

    - client/server TLS1.3 ECC example

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
wolfSSL source root directory. It will download `mbedtls` and compile it. It
will also build WolfSSL with correct PSA headers and options. To use the script
and then compile the examples use these commands:

```
cd /path/to/wolfSSL/src;
./path/to/wolfssl-examples/psa/build_with_mbedtls_psa.sh
make install
cd /path/to/wolfssl-examples/psa/
export PSA_INCLUDE=/tmp/mbedtls/build/include
export PSA_LIB_PATH=/tmp/mbedtls/build/library/libmbedcrypto.a 
make
```


