# BTLE Examples

Bluetooth Low Energy (BTLE or BLE) is a leightweight / low power wireless protocol. Its supported by Apple iPhone 4s and later and most Android phones. It operates in the 2.4GHz spectrum and has 3 advertising channels and 37 data channels.

These examples demonstrate leightweight methods for exchanging data securley over anytype of publically visible link.

The first phase is key establishment, which is done through some type of shared secret mechanism such as ECDH and a HKDF. ECC was choosen for these examples because its leightweight and widely used.

The second phase is encrypted data communication and data integrity.

## ECC Encrypt/Decrypt Example

See `BTLESecureMessageExchange.pdf` for details.

### Building

The wolfSSL library must be built and installed using './configure --enable-ecc --enable-eccencrypt && make && sudo make install' or by defining `#define HAVE_ECC` and `#defineHAVE_ECC_ENCRYPT`.

### Usage

Use two consoles and STDIN to exchange data between the client and server.

```
./ecc-server
./ecc-client
```

