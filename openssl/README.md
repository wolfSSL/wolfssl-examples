# OpenSSL Examples

This directory contains examples demonstrating the use of OpenSSL for various cryptographic operations.

## Software Bill of Materials (SBOM)

| Component | Description | Version |
|-----------|-------------|---------|
| OpenSSL   | Cryptography and SSL/TLS Toolkit | 3.0.x |
| C Standard Library | Standard C library | - |

## Examples

### ECC Signature Verification

The `ecc-verify-openssl.c` example demonstrates how to verify an ECC signature using fixed test values:
- Fixed ECC public key (single encoded value in uncompressed format)
- Fixed signature (DER format)
- Fixed data (pre-computed hash)

This example uses OpenSSL's EVP interface APIs for key creation and signature verification, making it compatible with OpenSSL providers. It specifically uses:
- `EVP_PKEY_fromdata()` with `OSSL_PARAM_BLD` for key creation
- `OSSL_PKEY_PARAM_GROUP_NAME` and `OSSL_PKEY_PARAM_PUB_KEY` parameters
- `EVP_PKEY_verify()` for signature verification

#### Building

```
make ecc-verify-openssl
```

#### Running

```
./ecc-verify-openssl
```

#### Expected Output

```
OpenSSL ECC Signature Verification Example
Using fixed test values for public key, signature, and data

Data to verify: "sample message for signing"

Data (pre-computed hash) (length=32):
E7 BD 43 AE E7 50 B0 DA  4B 3F 53 7E 15 2C 4A 74
07 76 40 37 A5 74 47 CA  CD 20 96 A5 D6 09 94 BA

Public Key (uncompressed format) (length=65):
04 2F BF B3 2C 9E 28 90  FD 74 2D 99 1F D4 3B 88
91 91 79 1E 18 33 7A D4  08 CB 12 CA 79 32 5E 88
DD 7D 20 A5 18 23 18 EC  9B D6 96 DD F4 11 28 D8
BF 1A 13 7D 24 46 C7 A5  AF 6A A6 0A FE F3 E8 61
0E

Signature (DER format) (length=71):
30 45 02 20 76 3B 61 45  9C F2 FC 3E 82 10 53 70
2C C2 4C 0E 37 8C 97 6F  C6 F8 3F 0F 0F F0 5B D8
52 03 B9 58 02 21 00 A4  0A 0A 3A F5 07 69 EE 74
0C BA 8F 9B 5F 45 30 F5  E5 A9 3E DE BB AB 14 A1
F3 BE CE 93 FB 5A 47

Signature verification successful!

Verification result: SUCCESS (ret = 0)
```
