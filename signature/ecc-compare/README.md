# ECC Signature Verification Comparison

This directory contains examples demonstrating ECC signature verification using both OpenSSL and wolfSSL libraries with the same test values.

## Software Bill of Materials (SBOM)

| Component | Description | Version |
|-----------|-------------|---------|
| OpenSSL   | Cryptography and SSL/TLS Toolkit | 3.0.x |
| wolfSSL   | Embedded SSL/TLS Library | 5.6.x |
| C Standard Library | Standard C library | - |

## Examples

### OpenSSL Low-level API Example

The `ecc-verify-openssl-low.c` example demonstrates how to verify an ECC signature using OpenSSL's low-level EC interfaces, specifically `ECDSA_verify()`. It uses:

- Fixed ECC public key (single encoded value in uncompressed format)
- Fixed signature (DER format)
- Fixed data (pre-computed hash)

### wolfSSL Example

The `ecc-verify-wolfssl.c` example demonstrates how to verify the same ECC signature using wolfSSL's `wc_ecc_verify_hash` function. It uses the same fixed test values as the OpenSSL example.

## Test Values

Both examples use the following fixed test values:

- **Public Key (uncompressed format)**: 
  ```
  042FBFB32C9E2890FD742D991FD43B889191791E18337AD408CB12CA79325E88DD7D20A5182318EC9BD696DDF41128D8BF1A137D2446C7A5AF6AA60AFEF3E8610E
  ```

- **Signature (DER format)**:
  ```
  30450220763B61459CF2FC3E821053702CC24C0E378C976FC6F83F0F0FF05BD85203B958022100A40A0A3AF50769EE740CBA8F9B5F4530F5E5A93EDEBBAB14A1F3BECE93FB5A47
  ```

- **Data (pre-computed hash)**:
  ```
  E7BD43AEE750B0DA4B3F537E152C4A7407764037A57447CACD2096A5D60994BA
  ```

## Building

```
make
```

This will build both examples.

## Running

### OpenSSL Example

```
./ecc-verify-openssl-low
```

### wolfSSL Example

```
./ecc-verify-wolfssl
```

## Expected Output

Both examples should output the test values and verification results, showing successful signature verification.
