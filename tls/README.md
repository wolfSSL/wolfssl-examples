# wolfSSL TLS Example Applications

This directory contains example applications demonstrating various wolfSSL TLS features.

## Building the Examples
```bash
make
```

## Configuration Requirements

Some examples require specific wolfSSL configuration options:

### PKCallback Examples (client-tls-pkcallback, server-tls-pkcallback)
- Requires: `--enable-pkcallbacks`
- Purpose: Enable public key callback functionality
- Error if not enabled: "PK not compiled in"

### TLS BIO and PKCS12 Examples (client-tls-bio, client-tls-pkcs12)
- Requires: `--enable-opensslextra` and `--enable-des3`
- Purpose: OpenSSL compatibility layer and DES3 for PKCS12
- Error if not enabled: "requires --enable-opensslextra" or "wolfSSL not configured with --enable-des3"

### CryptoCB Examples (client-tls-cryptocb, server-tls-cryptocb)
- Requires: `--enable-cryptocb`
- Purpose: Enable crypto callback interface
- Error if not enabled: "Please configure wolfSSL with --enable-cryptocb"

### WriteUp Examples (client-tls-writedup, server-tls-writedup)
- Requires: `--enable-writedup`
- Purpose: Enable file descriptor duplication
- Error if not enabled: "wolfSSL not configured with --enable-writedup"

### ECH Examples (client-ech, server-ech-local)
- Requires: `--enable-ech`
- Purpose: Enable Encrypted Client Hello support
- Error if not enabled: "Please build wolfssl with --enable-ech"

### TLS 1.3 Resume Example (client-tls13-resume)
- Requires: Client certificates (same as client-tls13)
- Files needed:
  - CERT_FILE: "../certs/client-cert.pem"
  - KEY_FILE: "../certs/client-key.pem"
  - CA_FILE: "../certs/ca-cert.pem"

## Hardware Dependencies

### UART Examples (client-tls-uart, server-tls-uart)
See [UART Requirements](uart_requirements.md) for detailed setup instructions.
- Requires physical UART hardware
- Device must be accessible at /dev/ttyUSB0
- Cannot be tested in virtual environments

## Troubleshooting

1. Missing Configuration
```bash
# Configure wolfSSL with all required features
cd ~/repos/wolfssl
./configure --enable-pkcallbacks \
           --enable-opensslextra \
           --enable-cryptocb \
           --enable-des3 \
           --enable-writedup \
           --enable-ech
make
sudo make install
```

2. Certificate Issues
- Ensure all required certificates are in ../certs/ directory
- Check file permissions
- Verify certificate paths in example code

3. Hardware Issues
- For UART examples, verify hardware connection
- Check device permissions
- Use alternative examples for testing when hardware is unavailable

## Example Categories

1. Basic TLS Examples
- client-tls / server-tls
- client-tls13 / server-tls13

2. Session Resumption
- client-tls-resume (TLS 1.2)
- client-tls13-resume (TLS 1.3)

3. Callback Examples
- client-tls-callback / server-tls-callback
- client-tls-cryptocb / server-tls-cryptocb
- client-tls-pkcallback / server-tls-pkcallback

4. Performance Examples
- client-tls-perf
- server-tls-epoll-perf
- server-tls-poll-perf

5. Special Features
- client-tls-ecdhe / server-tls-ecdhe
- client-tls-nonblocking / server-tls-nonblocking
- client-tls-writedup / server-tls-writedup
- memory-tls
