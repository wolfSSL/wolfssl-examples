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

## Quick Start

### 1. Enable I2C on Raspberry Pi

```bash
sudo raspi-config
# Navigate to: Interface Options -> I2C -> Enable
```

Reboot if prompted, then verify the STSAFE device is detected:

```bash
sudo i2cdetect -y 1
# STSAFE should appear at address 0x20
```

Optional: Allow non-root I2C access:
```bash
sudo usermod -a -G i2c $USER
# Logout and login again
```

### 2. Build wolfSSL

```bash
cd ~/wolfssl
./configure --enable-cryptocb --enable-ecc --enable-cmac --enable-sha384
make
```

### 3. Clone STSELib

```bash
git clone https://github.com/STMicroelectronics/STSELib.git ~/STSELib
```

### 4. Build and Run Tests

```bash
cd ~/wolfssl-examples/stsafe

# Build all test executables
make

# Run all tests
make test-all
```

## Test Suites

### Basic STSELib Tests (`stsafe_test`)

Tests core STSAFE-A120 functionality:
- Echo command (I2C communication)
- Random number generation
- ECC P-256 key generation
- ECDSA P-256 signing
- ECC P-384 key generation

```bash
make && ./stsafe_test
```

### wolfSSL Crypto Callback Tests (`wolfssl_stsafe_test`)

Tests wolfSSL crypto callbacks with STSAFE:
- RNG with STSAFE-A120
- ECC P-256/P-384 key generation via crypto callback
- ECDSA P-256/P-384 sign/verify
- ECDHE P-256 ephemeral key generation
- ECDHE P-256 shared secret computation

```bash
make wolfssl && ./wolfssl_stsafe_test
```

### Full Integration Tests (`wolfssl_stsafe_full_test`)

Comprehensive tests with benchmarks:
- RNG benchmark
- ECDSA P-256 benchmark (keygen, sign, verify timing)
- ECDH P-256 key exchange (uses ECDHE ephemeral keys)
- Multiple sequential operations

```bash
make wolfssl-full && ./wolfssl_stsafe_full_test
```

## Build Targets

| Target | Description |
|--------|-------------|
| `make` | Build all test executables |
| `make test-all` | Build and run all tests |
| `make basic` | Build without wolfSSL (basic I2C tests only) |
| `make clean` | Clean build artifacts |
| `make info` | Show configuration |

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
| ECDHE P-256 KeyGen | ~42 ms | ~24 ops/sec |
| ECDHE P-256 Shared Secret | ~38 ms | ~26 ops/sec |

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

## Environment Variables

Default paths assume `~/wolfssl` and `~/STSELib`. Override if needed:

```bash
export WOLFSSL_DIR=/path/to/wolfssl
export STSELIB_DIR=/path/to/STSELib
```

## Troubleshooting

### Error 0x0104 (STSE_PLATFORM_BUS_ERR)

I2C communication error. Check:
1. I2C is enabled: `ls /dev/i2c*`
2. Device detected: `sudo i2cdetect -y 1` (should show `20`)
3. Wiring connections are secure

### Build errors

Ensure wolfSSL is built with required features:
```bash
./configure --enable-cryptocb --enable-ecc --enable-cmac --enable-sha384
```

## References

- [wolfSSL Documentation](https://www.wolfssl.com/docs/)
- [STSELib GitHub](https://github.com/STMicroelectronics/STSELib)
- [STSAFE-A120 Datasheet](https://www.st.com/en/secure-mcus/stsafe-a120.html)
- [Raspberry Pi I2C Documentation](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html)
