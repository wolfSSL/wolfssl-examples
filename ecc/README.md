# Examples demonstrating ECC

## Building

### Build and install wolfSSL

```
./configure --enable-ecc --enable-ecccustcurves CFLAGS="-DWOLFSSL_TEST_CERT -DWOLFSSL_DER_TO_PEM -DHAVE_ECC_KOBLITZ" && make && sudo make install
```

### Build Example

```
make
gcc -o ecc-key-decode ecc-key-decode.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-key-decode ecc-key-decode.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-sign ecc-sign.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-stack ecc-stack.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-verify ecc-verify.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
```

### Debug

To enable debug change the Makefile to:

```
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
```

Build wolfSSL adding `--enable-debug` to the ./configure.

To enable using the static library change the Makefile to:

```
LIBS+=$(STATIC_LIB)
#LIBS+=$(DYN_LIB)
```

Build wolfSSL adding `--disable-shared` to the ./configure.


## Usage

### `ecc-key-decode`

This example shows how to extract a public key from a certificate and load it.

```
./ecc-key-decode
bytes = 781
decodedCert.pubKeySize 91
publickey size: 32
Success
```

### `ecc-sign`

This example demonstrates signing large data by performing the hash in chunks and signing. The sign is done multiple times to show how its normal for the signature size to vary for the same private key and hash data because a new public key is generated for each operation.

```
./ecc-sign
KeyGen Done
Sign ret 0, sigLen 103
Verify ret 0, is_valid_sig 1
Firmware Signature 0: Ret 0, HashLen 32, SigLen 103
Sign ret 0, sigLen 102
Verify ret 0, is_valid_sig 1
Firmware Signature 1: Ret 0, HashLen 32, SigLen 102
Sign ret 0, sigLen 102
Verify ret 0, is_valid_sig 1
Firmware Signature 2: Ret 0, HashLen 32, SigLen 102
Sign ret 0, sigLen 103
Verify ret 0, is_valid_sig 1
Firmware Signature 3: Ret 0, HashLen 32, SigLen 103
Sign ret 0, sigLen 103
Verify ret 0, is_valid_sig 1
Firmware Signature 4: Ret 0, HashLen 32, SigLen 103
Sign ret 0, sigLen 103
Verify ret 0, is_valid_sig 1
Firmware Signature 5: Ret 0, HashLen 32, SigLen 103
Sign ret 0, sigLen 102
Verify ret 0, is_valid_sig 1
Firmware Signature 6: Ret 0, HashLen 32, SigLen 102
Sign ret 0, sigLen 102
Verify ret 0, is_valid_sig 1
Firmware Signature 7: Ret 0, HashLen 32, SigLen 102
Sign ret 0, sigLen 103
Verify ret 0, is_valid_sig 1
Firmware Signature 8: Ret 0, HashLen 32, SigLen 103
Sign ret 0, sigLen 102
Verify ret 0, is_valid_sig 1
Firmware Signature 9: Ret 0, HashLen 32, SigLen 102
```

### `ecc-stack`

This example show stack use for an ECC key generation.

```
./ecc-stack
total   Allocs   =         5
total   Deallocs =         5
total   Bytes    =     16440
peak    Bytes    =     16440
current Bytes    =         0
stack used = 25296
sizeof RNG = 32
sizeof ecc_key = 4424
```

### `ecc-verify`

This example demonstrates using a Koblitz (SECP256K1) curve.

```
./ecc-verify
hash_firmware_verify: 0
```

### `ecc-key-decode`

This example shows exporting an ECC private key and public key.

```
./ecc-key-export
ECC Key Generated: 256 bits, curve ECC_SECP256K1
ECC Private Key Exported to ./ECC_SECP256K1.der
ECC Private Key Exported to ./ECC_SECP256K1.pem
ECC Public Key Exported to ./ECC_SECP256K1_pub.der
ECC Public Key Exported to ./ECC_SECP256K1_pub.pem
```

Example commands for parsing the generated ECC keys (see `./parsekeys.sh`):

```
openssl ec -inform der -in ECC_SECP256K1.der -text
openssl ec -inform pem -in ECC_SECP256K1.pem -text

openssl ec -inform der -in ECC_SECP256K1_pub.der -text -pubin
openssl ec -inform pem -in ECC_SECP256K1_pub.pem -text -pubin
```


## Support

For questions please email us at support@wolfssl.com.
