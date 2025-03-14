#!/bin/bash
# setup.sh
#
# Copyright (C) 2006-2024 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# wolfSSL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# wolfSSL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

# Exit on error
set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Create directories
mkdir -p freertos
mkdir -p include/FreeRTOSConfig

# Clone FreeRTOS repositories
cd freertos
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS.git
mkdir -p utils
cat > utils/utils.c << EOF
/* utils.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Utility functions for FreeRTOS */
void utils_init(void)
{
    /* Initialize random seed */
    srand(time(NULL));
}
EOF
cd ..

# Create TAP interface if it doesn't exist
if ! ip link show tap0 &>/dev/null; then
    echo "Creating TAP interface (requires sudo)..."
    sudo ip tuntap add dev tap0 mode tap
    sudo ip addr add 10.10.0.1/24 dev tap0
    sudo ip link set dev tap0 up
    echo "TAP interface created successfully."
else
    echo "TAP interface tap0 already exists."
fi

# Configure Mosquitto for testing
if [ -f "/etc/mosquitto/mosquitto.conf" ]; then
    echo "Checking Mosquitto configuration..."
    if ! grep -q "listener 8883" /etc/mosquitto/mosquitto.conf; then
        echo "Configuring Mosquitto for TLS (requires sudo)..."
        WOLFSSL_DIR=$(cd ../.. && cd ../wolfssl && pwd)
        
        # Create a temporary configuration file
        TMP_CONF=$(mktemp)
        cat > "$TMP_CONF" << EOF
# Added by wolfSSL example setup
listener 8883
allow_anonymous true
cafile $WOLFSSL_DIR/certs/ca-cert.pem
certfile $WOLFSSL_DIR/certs/server-cert.pem
keyfile $WOLFSSL_DIR/certs/server-key.pem
tls_version tlsv1.3
EOF
        
        # Append to mosquitto.conf
        sudo bash -c "cat $TMP_CONF >> /etc/mosquitto/mosquitto.conf"
        rm "$TMP_CONF"
        
        # Restart Mosquitto
        sudo systemctl restart mosquitto
        echo "Mosquitto configured for TLS on port 8883."
    else
        echo "Mosquitto already configured for TLS."
    fi
else
    echo "Mosquitto configuration not found. Please install Mosquitto MQTT broker."
fi

echo "Setup completed successfully!"
echo "To build the example:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make"
