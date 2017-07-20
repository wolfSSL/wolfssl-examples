# BTLE Examples

Bluetooth Low Energy (BTLE or BLE) is a leightweight / low power wireless protocol. Its supported by Apple iPhone 4s and later and most Android phones. It operates in the 2.4GHz spectrum and has 3 advertising channels and 37 data channels.

These examples demonstrate leightweight methods for exchanging data securley over anytype of publically visible link.

The first phase is key establishment, which is done through ECDH and HDKF. ECC was choosen for these examples because its leightweight and widely used. Salt exchanged to provent data reply of messages. The enryption is done with AES CBC. The data integrity is done using HMAC-SHA256.

## ECC Encrypt/Decrypt Example

See `BTLESecureMessageExchange.pdf` for details.

### Building

The wolfSSL library must be built and installed using './configure --enable-ecc --enable-eccencrypt --enable-hkdf && make && sudo make install' or by defining `#define HAVE_ECC`, `#define HAVE_ECC_ENCRYPT` and `HAVE_HKDF`.

### Usage

Use two consoles and STDIN to exchange data between the client and server.

```
./ecc-server
./ecc-client
```

### BTLE Simulator

The simulator uses IPC (pipes) to communicate between threads for simualted communication between two devices.
