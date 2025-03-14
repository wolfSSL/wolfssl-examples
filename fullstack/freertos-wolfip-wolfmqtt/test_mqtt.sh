#!/bin/bash

# Test script for wolfMQTT TLS example
# Tests MQTT functionality using mosquitto_sub with TLS

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

# Certificate paths (relative to wolfssl-examples repo)
CERT_PATH=$(realpath ../../../wolfssl/certs)

# Verify certificate files exist
for cert in "$CERT_PATH/ca-cert.pem" "$CERT_PATH/client-cert.pem" "$CERT_PATH/client-key.pem"; do
    if [ ! -f "$cert" ]; then
        echo "Error: Certificate file not found: $cert"
        exit 1
    fi
done

# Start mosquitto subscriber for testing with TLS
echo "Starting MQTT subscriber on test/topic with TLS..."
mosquitto_sub -h 10.10.0.1 -p 8883 \
    --cafile "$CERT_PATH/ca-cert.pem" \
    --cert "$CERT_PATH/client-cert.pem" \
    --key "$CERT_PATH/client-key.pem" \
    --tls-version tlsv1.3 \
    -t "test/topic" -v &
SUB_PID=$!

# Wait for messages
echo "Waiting for messages (10 seconds)..."
sleep 10

# Clean up
echo "Cleaning up..."
kill $SUB_PID

echo "Test complete"
