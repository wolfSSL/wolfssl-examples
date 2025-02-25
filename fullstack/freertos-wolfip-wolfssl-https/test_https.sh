#!/bin/bash

# Test HTTPS server with curl using wolfSSL test certificates

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

# Ensure TAP interface is up
if ! ip link show wtap0 >/dev/null 2>&1; then
    echo "TAP interface wtap0 not found. Please run setup_network.sh first."
    exit 1
fi

echo "Testing HTTPS server with curl..."
curl -v --cacert ./certs/ca-cert.pem \
     --tlsv1.3 --insecure https://10.10.0.10:443/

# Check if curl command succeeded
if [ $? -eq 0 ]; then
    echo "HTTPS test successful!"
else
    echo "HTTPS test failed!"
    exit 1
fi
