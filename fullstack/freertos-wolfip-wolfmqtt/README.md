# FreeRTOS + wolfIP + wolfMQTT TLS Example

This example demonstrates a full-stack embedded MQTT client using FreeRTOS, wolfIP, and wolfMQTT with TLS security. It shows how to establish a secure MQTT connection from an embedded device to a broker using wolfSSL for TLS.

## Software Bill of Materials (SBOM)

| Component | Version | License | Description |
|-----------|---------|---------|-------------|
| FreeRTOS | 10.5.1 | MIT | Real-time operating system for embedded devices |
| wolfSSL | 5.6.3 | GPLv2 | Embedded SSL/TLS library |
| wolfMQTT | 1.16.0 | GPLv2 | Embedded MQTT client library |
| wolfIP | 1.0.0 | GPLv2 | Embedded TCP/IP stack |
| Mosquitto | 2.0.15 | EPL/EDL | MQTT broker for testing |

## Prerequisites

- Linux development environment
- CMake 3.13 or later
- GCC compiler
- Mosquitto MQTT broker
- Root/sudo privileges (for TAP interface setup)

## Setup

1. Clone the required repositories:
   ```
   ./setup.sh
   ```

   This script will:
   - Clone FreeRTOS kernel
   - Clone FreeRTOS repository
   - Create necessary directories
   - Configure the build environment

2. Configure the TAP interface on your host system:
   ```
   sudo ip tuntap add dev tap0 mode tap
   sudo ip addr add 10.10.0.1/24 dev tap0
   sudo ip link set dev tap0 up
   ```

3. Configure Mosquitto broker with TLS:
   ```
   sudo nano /etc/mosquitto/mosquitto.conf
   ```

   Add the following configuration:
   ```
   listener 8883
   allow_anonymous true
   cafile /path/to/wolfssl/certs/ca-cert.pem
   certfile /path/to/wolfssl/certs/server-cert.pem
   keyfile /path/to/wolfssl/certs/server-key.pem
   tls_version tlsv1.3
   ```

   Restart Mosquitto:
   ```
   sudo systemctl restart mosquitto
   ```

## Building the Example

1. Create a build directory:
   ```
   mkdir build && cd build
   ```

2. Configure with CMake:
   ```
   cmake ..
   ```

3. Build the example:
   ```
   make
   ```

## Running the Example

1. Run the FreeRTOS simulator:
   ```
   ./freertos_sim
   ```

2. The application will:
   - Initialize the FreeRTOS kernel
   - Configure the TAP interface with IP 10.10.0.10
   - Establish a TLS connection to the Mosquitto broker at 10.10.0.1:8883
   - Connect to the MQTT broker
   - Subscribe to the test topic
   - Publish messages to the test topic
   - Process incoming messages

3. Monitor the MQTT traffic with Mosquitto client:
   ```
   mosquitto_sub -h 10.10.0.1 -p 8883 -t "test/topic" --cafile /path/to/wolfssl/certs/ca-cert.pem --insecure
   ```

## Troubleshooting

1. Verify TAP interface is up and configured:
   ```
   ip addr show tap0
   ```

2. Check Mosquitto broker is running:
   ```
   sudo systemctl status mosquitto
   ```

3. Verify Mosquitto is listening on the correct port:
   ```
   sudo netstat -tulpn | grep mosquitto
   ```

4. Run Mosquitto in verbose mode for debugging:
   ```
   mosquitto -c /etc/mosquitto/mosquitto.conf -v
   ```

## License

This example is licensed under the GPLv2 license. See the LICENSE file for details.
