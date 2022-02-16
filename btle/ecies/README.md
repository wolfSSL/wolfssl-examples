# BTLE ECIES Example

This example demonstrates a lightweight method for exchanging data securely over any type of publicly visible link.

The first phase is key establishment, which is done through ECDH and HDKF. ECC was chosen for these examples because its lightweight and widely used. 
Then salt exchanged for each message to prevent reply attacks. The encryption is done with AES CBC. The data integrity is done using HMAC-SHA256.

The peer's key should be validated against a known key or certificate. Client should hash and verify this public key against trusted certificate (already exchanged) out of band. An ECC signature is about 65 bytes.

## ECC Encrypt/Decrypt Example

See `BTLESecureMessageExchange.pdf` for details.


### Building

The wolfSSL library must be built and installed using:

```
./configure --enable-eccencrypt
make
make check
sudo make install
```
If downloaded from GitHub you'll also need to run `./autogen.sh`.

 OR by defining:
 
```
#define HAVE_ECC
#define HAVE_ECC_ENCRYPT
#define HAVE_HKDF
```

### Usage

Use two consoles and STDIN to exchange data between the client and server. 

From the client enter a message and hit enter. This will be encrypted and sent to the server. The server will decrypt, print and re-encrypt the message and send it back to the client (echo).

Type "exit" to close the connection.

```
% ./ecc-server
Waiting for client
Recv 16: asdf
Recv 16: exit
Exit, closing connection
```

```
% ./ecc-client
Enter text to send:
asdf
Recv 16: asdf
Enter text to send:
exit
Recv 16: exit
Exit, closing connection
```

Note: The messages are padded to the AES block size 16-bytes.

### Debugging

To enable debugging or switch to using a static version of wolfSSL edit the `Makefile` and uncomment `CFLAGS+=$(DEBUG_FLAGS)` and `STATIC_LIB+=$(LIB_PATH)/lib/libwolfssl.a`. Then comment out `LIBS+=$(DYN_LIB) -lm`.
