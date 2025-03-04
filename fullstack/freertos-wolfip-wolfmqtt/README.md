# FreeRTOS + wolfIP + wolfMQTT TLS Example

This example demonstrates a full-stack embedded MQTT client using FreeRTOS, wolfIP, and wolfMQTT with TLS 1.3 security.

## Stack Components
- FreeRTOS (POSIX port) - Real-time operating system
- wolfIP - TCP/IP networking stack
- wolfMQTT - MQTT client implementation
- wolfSSL - TLS 1.3 security layer
- TAP interface - Virtual network interface

## Prerequisites
- Linux with TUN/TAP support
- CMake (>= 3.13)
- GCC
- Mosquitto broker (for testing)
- wolfSSL libraries

## Building Dependencies

### wolfSSL
```bash
cd ~/repos/wolfssl
./autogen.sh
./configure --enable-tls13 --enable-aead --enable-hkdf \
    --enable-certgen --enable-certreq --enable-certext \
    --enable-tlsx --enable-snl --enable-ocsp \
    --enable-cryptocb --enable-des3 --enable-curve25519 \
    --enable-ed25519 --enable-keygen --enable-rsapss \
    --enable-sha224 --enable-sha384 --enable-sha512 \
    --enable-dh --enable-opensslextra
make
sudo make install
```

### wolfMQTT
```bash
cd ~/repos/wolfMQTT
./autogen.sh
./configure --enable-tls --enable-nonblock
make
sudo make install
```

## Building and Running

### Setup
1. Run the setup script to clone and configure dependencies:
```bash
./setup.sh
```

2. Configure the network interface (requires root):
```bash
sudo ./setup_network.sh
```

3. Build the example:
```bash
cd build && cmake .. && make
```

4. Run the example (requires root):
```bash
sudo ./freertos_sim
```

### Testing
Test the MQTT client using the provided script:
```bash
sudo ./test_mqtt.sh
```

## Network Configuration
- TAP Interface: 10.10.0.1/24 (Host/Broker)
- FreeRTOS IP: 10.10.0.10/24
- Default Gateway: 10.10.0.1
- MQTT Broker Port: 8883 (TLS)

## Security Features
- TLS 1.3 with wolfSSL
- Certificate-based authentication
- Secure MQTT over TLS

## Software Bill of Materials (SBOM)
| Component | Version | License | Source |
|-----------|---------|----------|---------|
| FreeRTOS | Latest | MIT | https://github.com/FreeRTOS/FreeRTOS |
| FreeRTOS-Kernel | Latest | MIT | https://github.com/FreeRTOS/FreeRTOS-Kernel |
| wolfSSL | Latest | GPLv2 | https://github.com/wolfSSL/wolfssl |
| wolfIP | Latest | GPLv2 | https://github.com/wolfSSL/wolfip |
| wolfMQTT | Latest | GPLv2 | https://github.com/wolfSSL/wolfMQTT |
