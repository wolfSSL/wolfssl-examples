# BTLE Examples

BTLE / Lightweight Secure Transport Example:

Bluetooth Low Energy (BTLE) is a lightweight / low power wireless protocol. Its supported by Apple iPhone 4s and later and most Android phones. It operates in the 2.4GHz spectrum and has 3 advertising channels and 37 data channels.

This example demonstrates a lightweight method for exchanging data securely over any type of publicly visible link.

The first phase is key establishment, which is done through ECDH and HDKF. ECC was chosen for these examples because its lightweight and widely used. Then salt exchanged for each message to prevent reply attacks. The encryption is done with AES CBC. The data integrity is done using HMAC-SHA256.


## ECC Encrypt/Decrypt Example

See `BTLESecureMessageExchange.pdf` for details.


### Building

The wolfSSL library must be built and installed using './configure --enable-ecc --enable-eccencrypt --enable-hkdf && make && sudo make install' or by defining `#define HAVE_ECC`, `#define HAVE_ECC_ENCRYPT` and `HAVE_HKDF`.


### Usage

Use two consoles and STDIN to exchange data between the client and server. From the client enter a message and hit enter. This will be encrypted and sent to the server. The server will decrypt, print and re-encrypt the message and send it back to the client (echo).

```
./ecc-server
./ecc-client
```

### Debugging

To enable debugging or switch to using a static version of wolfSSL edit the `Makefile` and uncomment `CFLAGS+=$(DEBUG_FLAGS)` and `STATIC_LIB+=$(LIB_PATH)/lib/libwolfssl.a`. Then comment out `LIBS+=$(DYN_LIB) -lm`.


## BTLE Simulator

The simulator uses IPC (pipes) to communicate between threads for simulated communication between two devices.
