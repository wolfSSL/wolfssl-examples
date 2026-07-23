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

# The sim's cooperative scheduler can be slow to bind :443 on a loaded host, so
# retry rather than fail on a single connection refusal.
attempts=30
for i in $(seq 1 "$attempts"); do
    curl -v --cacert ./certs/ca-cert.pem \
         --tlsv1.3 --insecure https://10.10.0.10:443/ && break
    if [ "$i" -eq "$attempts" ]; then
        echo "HTTPS test failed!"
        exit 1
    fi
    sleep 1
done

echo "HTTPS test successful!"
