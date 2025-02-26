#!/bin/bash
# Configure host TAP interface for wolfSSL embedded testing
# Creates a TAP interface for virtual networking between host and FreeRTOS

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

# Remove existing interface if present
ip link show wtap0 >/dev/null 2>&1 && ip link delete wtap0

# Create new TAP interface and configure it
ip tuntap add dev wtap0 mode tap
ip link set wtap0 down
ip addr flush dev wtap0
ip addr add 10.10.0.1/24 dev wtap0
ip link set wtap0 up

echo "TAP interface wtap0 configured with IP 10.10.0.1/24"
