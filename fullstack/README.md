# wolfSSL Full-Stack Examples

This directory contains examples demonstrating full-stack implementations using wolfSSL components on FreeRTOS.

## Examples

### 1. FreeRTOS + wolfIP + wolfSSL HTTPS Server
Located in `freertos-wolfip-wolfssl-https/`

Demonstrates:
- HTTPS server implementation using wolfSSL
- FreeRTOS POSIX simulation
- wolfIP network stack integration
- TLS 1.3 support
- Network configuration using TAP interface

See the example's README.md for detailed setup and usage instructions.

### 2. FreeRTOS + wolfIP + wolfMQTT Client
Located in `freertos-wolfip-wolfmqtt/`

Demonstrates:
- MQTT client with TLS 1.3 using wolfMQTT
- FreeRTOS POSIX simulation
- wolfIP network stack integration
- Secure MQTT over TLS
- Network configuration using TAP interface

See the example's README.md for detailed setup and usage instructions.

## Common Requirements
- Linux environment
- Root access (for TAP interface setup)
- CMake 3.13+
- GCC
- wolfSSL libraries
- FreeRTOS source (automatically downloaded by setup scripts)

## Network Configuration
Both examples use a TAP interface for network connectivity:
- Host IP (Linux): 10.10.0.1/24
- FreeRTOS client/server: 10.10.0.10/24

## Building and Running
Each example contains:
- `setup.sh`: Downloads and configures FreeRTOS
- `setup_network.sh`: Configures TAP interface
- `CMakeLists.txt`: Build configuration
- Test scripts for verification

Follow the README.md in each example directory for specific instructions.
