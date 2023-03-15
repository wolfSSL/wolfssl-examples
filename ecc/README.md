# Examples demonstrating ECC

## Building

### Build and install wolfSSL

```
./configure --enable-ecc --enable-ecccustcurves CFLAGS="-DWOLFSSL_TEST_CERT -DWOLFSSL_DER_TO_PEM -DHAVE_ECC_KOBLITZ -DWOLFSSL_PUBLIC_MP" && make && sudo make install
```

### Build Example

```
make
gcc -o ecc-key-decode ecc-key-decode.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-key-decode ecc-key-decode.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-params ecc-params.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
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

### `ecc-key-export`

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

### `ecc-params`

Example for extracting curve parameters for a specific ECC curve.

```
./ecc-params
ECC Curve Parameters for SECP256R1
Prime: 32
	ff ff ff ff 00 00 00 01 00 00 00 00 00 00 00 00 |................
	00 00 00 00 ff ff ff ff ff ff ff ff ff ff ff ff |................
Af: 32
	ff ff ff ff 00 00 00 01 00 00 00 00 00 00 00 00 |................
	00 00 00 00 ff ff ff ff ff ff ff ff ff ff ff fc |................
Bf: 32
	5a c6 35 d8 aa 3a 93 e7 b3 eb bd 55 76 98 86 bc |Z.5..:.....Uv...
	65 1d 06 b0 cc 53 b0 f6 3b ce 3c 3e 27 d2 60 4b |e....S..;.<>'.`K
Order: 32
	ff ff ff ff 00 00 00 00 ff ff ff ff ff ff ff ff |................
	bc e6 fa ad a7 17 9e 84 f3 b9 ca c2 fc 63 25 51 |.............c%Q
Gx: 32
	6b 17 d1 f2 e1 2c 42 47 f8 bc e6 e5 63 a4 40 f2 |k....,BG....c.@.
	77 03 7d 81 2d eb 33 a0 f4 a1 39 45 d8 98 c2 96 |w.}.-.3...9E....
Gy: 32
	4f e3 42 e2 fe 1a 7f 9b 8e e7 eb 4a 7c 0f 9e 16 |O.B........J|...
	2b ce 33 57 6b 31 5e ce cb b6 40 68 37 bf 51 f5 |+.3Wk1^...@h7.Q.
```

### `ecc-export-qx-qy`

Example for extracting public key parameters from ASN.1 format to raw ECC point.
The raw ECC point format, also used by [wolfBoot](https://github.com/wolfSSL/wolfBoot), represents a public key in its two
coordinates (Qx,Qy) of fixed size (2 x keysize).

Usage:

`./ecc-export-Qx-Qy der_key_file raw_key_file`

Where `der_key_file` is a file containing the ecc key in ASN.1 format, and `raw_key_file` is the output file created, containing the public key in raw ECC point format.

## Support

For questions please email us at support@wolfssl.com.
