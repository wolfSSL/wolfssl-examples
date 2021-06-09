# wolfSSL X509 Field Extraction Example

Example of parsing a DER encoded self-signed certificate and extracting
public key and subject name information.

## Compiling and Running the Example

To compile, first build wolfSSL with the OpenSSL compatibility layer enabled:

```
$ cd wolfssl-X.X.X
$ ./configure --enable-opensslextra
$ make
$ sudo make install
```

Then, compile the example app:

```
$ make
$ ./app
```

For support, please contact support@wolfssl.com
