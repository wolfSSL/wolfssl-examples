# BTLE TLS v1.3 Example

This example demonstrates using TLS v1.3 to exchange data securely over a BTLE type transport link.

### Building

The wolfSSL library must be built and installed using:

```
./configure --enable-tls13
make
make check
sudo make install
```

If downloaded from GitHub you'll also need to run `./autogen.sh`.

### Usage

Use two consoles to run the examples to send a test message securely. The server will present an ECC server certificate and the client will validate it.

The client will attempt STDIN input and the server will echo. Use "exit" to close the connection.

```
% ./server-tls13-btle
Waiting for client
TLS accepting
TLS Accept handshake done
Read (5): asdf
Sent (5): asdf
Read (5): exit
Sent (5): exit
Exit, closing connection
```

```
% ./client-tls13-btle
TLS connecting
TLS Connect handshake done
Enter text to send:
asdf
Sent (5): asdf
Read (5): asdf
Enter text to send:
exit
Sent (5): exit
Read (5): exit
Exit, closing connection
```

### Debugging

To enable debugging or switch to using a static version of wolfSSL edit the `Makefile` and uncomment `CFLAGS+=$(DEBUG_FLAGS)` and `STATIC_LIB+=$(LIB_PATH)/lib/libwolfssl.a`. Then comment out `LIBS+=$(DYN_LIB) -lm`.
