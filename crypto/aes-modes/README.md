# AES Mode Examples

This directory contains examples demonstrating all 16 AES modes supported by
wolfSSL's wolfCrypt library. Each example encrypts a file using the one-shot
API and decrypts it using the streaming API (where available).

## Overview

Each example demonstrates:
- One-shot encryption using the mode's encrypt function
- Streaming decryption using Init/Update/Final pattern (if available)
- Proper key/IV/nonce generation and handling
- File I/O for practical usage

## AES Modes

| Mode | File | Streaming Decrypt | Build Flag | Description |
|------|------|-------------------|------------|-------------|
| CBC | aes-cbc.c | No | HAVE_AES_CBC | Cipher Block Chaining |
| CFB | aes-cfb.c | Yes | WOLFSSL_AES_CFB | Cipher Feedback (128-bit) |
| CFB1 | aes-cfb1.c | Yes | WOLFSSL_AES_CFB | Cipher Feedback (1-bit) |
| CFB8 | aes-cfb8.c | Yes | WOLFSSL_AES_CFB | Cipher Feedback (8-bit) |
| OFB | aes-ofb.c | Yes | WOLFSSL_AES_OFB | Output Feedback |
| ECB | aes-ecb.c | No | HAVE_AES_ECB | Electronic Codebook |
| CTR | aes-ctr.c | Yes | WOLFSSL_AES_COUNTER | Counter Mode |
| DIRECT | aes-direct.c | No | WOLFSSL_AES_DIRECT | Raw Block Cipher |
| GCM | aes-gcm.c | Yes* | HAVE_AESGCM | Galois/Counter Mode (AEAD) |
| GMAC | aes-gmac.c | No | HAVE_AESGCM | Galois MAC (auth only) |
| CCM | aes-ccm.c | No | HAVE_AESCCM | Counter with CBC-MAC (AEAD) |
| KEY WRAP | aes-keywrap.c | No | HAVE_AES_KEYWRAP | RFC 3394 Key Wrap |
| XTS | aes-xts.c | Yes* | WOLFSSL_AES_XTS | XEX-based Tweaked-codebook |
| SIV | aes-siv.c | No | WOLFSSL_AES_SIV | Synthetic IV (AEAD) |
| EAX | aes-eax.c | Yes | WOLFSSL_AES_EAX | Encrypt-Authenticate-Translate |
| CTS | aes-cts.c | No* | WOLFSSL_AES_CTS | Ciphertext Stealing |

*GCM streaming requires WOLFSSL_AESGCM_STREAM, XTS streaming requires
WOLFSSL_AESXTS_STREAM, CTS streaming API requires complex internal buffering
and is not demonstrated in this example

## Building

### Prerequisites

wolfSSL must be installed with the required AES modes enabled. To enable all
modes:

```bash
cd /path/to/wolfssl
./autogen.sh
./configure --enable-aescbc \
            --enable-aescfb \
            --enable-aesofb \
            --enable-aesecb \
            --enable-aesctr \
            --enable-aesgcm \
            --enable-aesccm \
            --enable-aeskeywrap \
            --enable-xts \
            --enable-aessiv \
            --enable-aeseax \
            --enable-aescts \
            --enable-aesgcm-stream \
            --enable-aesxts-stream
make
sudo make install
```

### Building the Examples

```bash
make
```

## Usage

All examples follow the same pattern:

```bash
./<example> <input_file> <output_file>
```

The example will:
1. Read the input file
2. Encrypt it using the one-shot API
3. Write encrypted data to a temporary file
4. Decrypt using streaming API (or one-shot if no streaming available)
5. Write decrypted data to the output file
6. Clean up temporary files

### Example

```bash
# Create a test file
echo "Hello, wolfSSL AES modes!" > test.txt

# Test AES-GCM
./aes-gcm test.txt output.txt
cat output.txt

# Test AES-CTR
./aes-ctr test.txt output.txt
cat output.txt
```

## Notes

### Security Considerations

- These examples use fixed keys for demonstration purposes only
- In production, use proper key management and secure random key generation
- ECB mode is not recommended for most use cases due to security weaknesses
- GMAC provides authentication only (no encryption)

### Streaming vs One-Shot

Modes with streaming support allow processing data in chunks, which is useful
for:
- Large files that don't fit in memory
- Network streams where data arrives incrementally
- Memory-constrained environments

Modes without streaming support require the entire plaintext/ciphertext to be
available before processing.

### Minimum Input Sizes

Some modes have minimum input size requirements:
- XTS: Minimum 16 bytes
- CTS: Minimum 16 bytes
- KEY WRAP: Input must be multiple of 8 bytes (padding applied automatically)
