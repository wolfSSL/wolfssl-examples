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
- Fixed signature (single buffer with R|S components)
- Fixed data that was signed

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

Signature verification successful!

Verification result: SUCCESS (ret = 0)
```
