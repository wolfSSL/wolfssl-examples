# BTLE TLS v1.3 Example

This example demonstrates using TLS v1.3 to exchange data securely over a BTLE type transport link.

### Building

The wolfSSL library must be built and installed using:

```
./configure --enable-tls13
make
sudo make install
```

### Usage

Use two consoles run the examples to send a test message securely. The server will present an ECC server certificate and the client will validate it.

```
% ./server-tls13-btle
Waiting for client
TLS accepting
TLS Accept handshake done
Read (0): Testing 1, 2 and 3
Sent (0): Testing 1, 2 and 3
```

```
 % ./client-tls13-btle
TLS connecting
TLS Connect handshake done
Sending test string
Sent (0): Testing 1, 2 and 3
Read (0): Testing 1, 2 and 3
```

### Debugging

To enable debugging or switch to using a static version of wolfSSL edit the `Makefile` and uncomment `CFLAGS+=$(DEBUG_FLAGS)` and `STATIC_LIB+=$(LIB_PATH)/lib/libwolfssl.a`. Then comment out `LIBS+=$(DYN_LIB) -lm`.
