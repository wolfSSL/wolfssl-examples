# UART Example Hardware Requirements

## Required Hardware
1. Physical UART device (e.g., USB-to-UART converter)
2. Device must be accessible at /dev/ttyUSB0

## Setup
1. Connect UART device to USB port
2. Verify device is recognized:
   ```bash
   ls -l /dev/ttyUSB0
   ```
3. Ensure user has proper permissions:
   ```bash
   sudo usermod -a -G dialout $USER
   ```

## Testing Without Hardware
The UART examples cannot be tested without physical hardware. For development and testing purposes:
1. Use the standard TLS examples (client-tls/server-tls) instead
2. Use TCP examples (client-tcp/server-tcp) for basic communication testing
3. Use memory-based examples (memory-tls) for protocol testing

## Error Messages
Common error: "Error opening /dev/ttyUSB0: Error 2 (No such file or directory)"
- Indicates missing UART hardware
- Expected in virtual/CI environments
- Not a code issue, requires physical hardware
