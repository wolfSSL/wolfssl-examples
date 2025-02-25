#!/bin/bash

# Test script for wolfMQTT TLS example
# Tests MQTT functionality using mosquitto_sub with TLS

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

# Certificate paths (relative to wolfssl-examples repo)
CA_CERT="../../../wolfssl/certs/ca-cert.pem"
CLIENT_CERT="../../../wolfssl/certs/client-cert.pem"
CLIENT_KEY="../../../wolfssl/certs/client-key.pem"

# Verify certificate files exist
for cert in "$CA_CERT" "$CLIENT_CERT" "$CLIENT_KEY"; do
    if [ ! -f "$cert" ]; then
        echo "Error: Certificate file not found: $cert"
        exit 1
    fi
done

# Start mosquitto subscriber for testing with TLS
echo "Starting MQTT subscriber on test/topic with TLS..."
mosquitto_sub -h 10.10.0.1 -p 8883 \
    --cafile "$CA_CERT" \
    --cert "$CLIENT_CERT" \
    --key "$CLIENT_KEY" \
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
