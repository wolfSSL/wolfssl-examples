# wolfSSL STSAFE-A120 Test Suite

Test harness for wolfSSL integration with ST STSAFE-A120 secure element on Raspberry Pi 5.

## Hardware Requirements

- Raspberry Pi 5 (or compatible Linux system with I2C)
- STSAFE-A120 secure element connected via I2C
- I2C enabled on the system

## Software Requirements

- wolfSSL library (compiled with ECC, CMAC, SHA-384 support)
- STSELib (ST Secure Element Library)
- GCC compiler
- Linux I2C development headers (`libi2c-dev`)

## Directory Structure

```
stsafe/
├── Makefile                          # Build configuration
├── README.md                         # This file
├── user_settings.h                   # wolfSSL configuration
├── stsafe_test.c                     # STSELib basic tests
├── wolfssl_stsafe_test.c             # wolfSSL crypto callback tests
├── wolfssl_stsafe_full_test.c        # Full integration tests with benchmarks
└── platform/
    ├── stse_conf.h                   # STSELib configuration
    ├── stse_platform_generic.h       # Platform type definitions
    ├── stse_platform_linux.c         # Linux I2C platform implementation
    └── stse_platform_crypto_wolfssl.c # wolfSSL crypto for STSELib
```

## Configuration

### I2C Setup

1. Enable I2C on Raspberry Pi:
   ```bash
   sudo raspi-config
   # Navigate to: Interface Options -> I2C -> Enable
   ```

2. Verify I2C device:
   ```bash
   sudo i2cdetect -y 1
   # STSAFE should appear at address 0x20
   ```

3. Set I2C permissions (optional, for non-root access):
   ```bash
   sudo usermod -a -G i2c $USER
   # Logout and login again
   ```

### Environment

Set paths to dependencies:
```bash
export WOLFSSL_DIR=$HOME/wolfssl
export STSELIB_DIR=$HOME/STSELib
```

## Building

### Prerequisites

1. Build wolfSSL with required features:
   ```bash
   cd $WOLFSSL_DIR
   ./configure --enable-cryptocb --enable-ecc --enable-cmac --enable-sha384 --enable-debug
   make
   ```

2. Clone STSELib:
   ```bash
   git clone https://github.com/STMicroelectronics/STSELib.git $STSELIB_DIR
   ```

### Build Targets

```bash
# Build basic STSELib tests (no wolfSSL crypto callbacks)
make

# Build wolfSSL crypto callback tests
make wolfssl

# Build full integration tests with benchmarks
make wolfssl-full

# Build without wolfSSL (basic I2C tests only)
make basic

# Clean build artifacts
make clean

# Show configuration
make info
```

## Running Tests

### Basic STSELib Tests
```bash
./stsafe_test
```

Tests:
- Echo command (I2C communication)
- Random number generation
- ECC P-256 key generation
- ECDSA P-256 signing
- ECC P-384 key generation

### wolfSSL Crypto Callback Tests
```bash
./wolfssl_stsafe_test
```

Tests:
- RNG with STSAFE-A120
- ECC P-256 key generation via crypto callback
- ECC P-384 key generation via crypto callback
- ECDSA P-256 sign/verify
- ECDSA P-384 sign/verify

### Full Integration Tests
```bash
./wolfssl_stsafe_full_test
```

Tests:
- RNG benchmark
- ECDSA P-256 benchmark (keygen, sign, verify timing)
- Multiple sequential operations

## Expected Output

```
================================================
STSAFE-A120 Test Suite for wolfSSL Integration
================================================

Initializing STSAFE handler...
STSAFE-A120 initialized successfully.

Test: Echo Command
  Echo response matches!
[PASS] Echo command

Test: Random Number Generation
  Random data: A1 B2 C3 D4 ...
[PASS] Random number generation

Test: ECC Key Generation (P-256)
  Public Key X: 12345678...
  Public Key Y: ABCDEF01...
[PASS] ECC P-256 key generation

...

================================================
Test Summary: 5 passed, 0 failed
================================================
```

## Performance Results (Raspberry Pi 5)

| Operation | Time | Throughput |
|-----------|------|------------|
| RNG (256 bytes) | <1 ms | ~9 MB/s |
| ECC P-256 KeyGen | ~40 ms | 25 ops/sec |
| ECDSA P-256 Sign | ~51 ms | 19.5 ops/sec |
| ECDSA P-256 Verify | ~79 ms | 12.7 ops/sec |

## References

- [wolfSSL Documentation](https://www.wolfssl.com/docs/)
- [STSELib GitHub](https://github.com/STMicroelectronics/STSELib)
- [STSAFE-A120 Datasheet](https://www.st.com/en/secure-mcus/stsafe-a120.html)
- [Raspberry Pi I2C Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#gpio-and-the-40-pin-header)
