# FreeRTOS + wolfIP + wolfSSL HTTPS Example

This example demonstrates a full-stack embedded networking application using FreeRTOS, wolfIP, and wolfSSL. It implements a secure HTTPS server running on a simulated FreeRTOS environment with TLS 1.3 support.

## Stack Components

The example integrates the following components:
- FreeRTOS (POSIX port) - Real-time operating system
- wolfIP - TCP/IP networking stack
- wolfSSL - TLS 1.3 security layer
- TAP interface - Virtual network interface

## Building and Running

### Prerequisites
- wolfSSL library
- wolfIP library
- CMake (>= 3.13)
- GCC
- Linux with TUN/TAP support

### Setup
1. Run the setup script to clone FreeRTOS repositories:
```bash
./setup.sh
```

2. Configure the network interface (requires root):
```bash
sudo ./setup_network.sh
```

3. Build the example:
```bash
mkdir -p build && cd build && cmake .. && make
```

4. Run the example (requires root):
```bash
sudo ./freertos_sim
```

### Testing
Test the HTTPS server using curl:
```bash
sudo ./test_https.sh
```

Or manually:
```bash
curl -v --cacert /path/to/wolfssl/certs/ca-cert.pem \
     --tlsv1.3 --insecure https://10.10.0.10:443/
```

## Software Bill of Materials (SBOM)

| Component | Version | License | Source |
|-----------|---------|----------|---------|
| FreeRTOS | Latest | MIT | https://github.com/FreeRTOS/FreeRTOS |
| FreeRTOS-Kernel | Latest | MIT | https://github.com/FreeRTOS/FreeRTOS-Kernel |
| wolfSSL | Latest | GPLv2 | https://github.com/wolfSSL/wolfssl |
| wolfIP | Latest | GPLv2 | https://github.com/wolfSSL/wolfip |

## Features
- TLS 1.3 support with wolfSSL
- Zero dynamic memory allocation networking with wolfIP
- Virtual networking through TAP interface
- UDP echo server for testing
- HTTPS server with demo page
- FreeRTOS task management and scheduling

## Network Configuration
- TAP Interface: 10.10.0.1/24 (Host)
- FreeRTOS IP: 10.10.0.10/24
- Default Gateway: 10.10.0.1

## Security Features
- TLS 1.3 with modern cipher suites
- Certificate-based authentication
- Support for various cryptographic algorithms:
  - AES (ECB, CBC, GCM)
  - ChaCha20-Poly1305
  - Curve25519
  - ED25519
  - SHA-2 and SHA-3 family
