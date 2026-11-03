# Mbed TLS Compatibility Example

`client-tls-mbedtls.c` is an application written for the Mbed TLS 4.x API,
built on wolfSSL through the Mbed TLS compatibility layer in
[wolfPSA](https://github.com/wolfSSL/wolfpsa). Its `tls_get()` function is the
client from the "Porting a TLS Client" chapter of the Mbed TLS to wolfSSL
migration guide, unchanged.

wolfPSA provides the PSA Crypto API on wolfCrypt, as TF-PSA-Crypto does for
Mbed TLS. Built with `MBEDTLS_COMPAT=1`, it also provides the Mbed TLS TLS,
X.509 and network API on wolfSSL, so the client links `libwolfpsa.a` in place
of `libmbedtls`, `libmbedx509` and `libtfpsacrypto`. The calls mapped so far
and the known differences from Mbed TLS are in wolfPSA's README, section
"Mbed TLS compatibility".

## Building

Build wolfPSA with the compatibility layer, next to `wolfssl-examples`. It
compiles wolfCrypt and the wolfSSL TLS sources from `WOLFSSL_PATH` into
`libwolfpsa.a` with its own settings, so the client needs no wolfSSL install:

```
cd wolfpsa
make MBEDTLS_COMPAT=1 WOLFSSL_PATH=../wolfssl
```

Then build the example (set `WOLFPSA_DIR` if wolfPSA is elsewhere):

```
make
```

The Makefile puts wolfPSA's headers on the include path, so `<mbedtls/ssl.h>`
resolves to wolfPSA even if Mbed TLS is installed too.

## Running

The servers are wolfSSL programs, built from `../tls/server-tls.c` against an
installed wolfSSL, as for the other examples:

```
cd wolfssl
./configure
make
sudo make install
sudo ldconfig
```

Start a TLS 1.3 server, then run the client:

```
make check-server-tls13
./check-server-tls13 &
./client-tls-mbedtls 127.0.0.1
```

```
tls_get() returned 0
```

The client checks the server certificate against `../certs/ca-cert.pem` and
the name `example.com`. Options:

* `-A <file>` CA certificate file, PEM or DER
* `-S <name>` server name to send and check
* `-N` use a non-blocking socket

## Testing

`make check` builds `../tls/server-tls.c` as `check-server-tls12` and
`check-server-tls13` and runs the client against them for TLS 1.2 and 1.3,
blocking and non-blocking, and for each failure it should report: wrong server
name, wrong CA, missing CA file and a CA file that is not a certificate.
`../tls/server-tls13` is not used because it requires a client certificate,
which this client does not send.

A wrong server name or CA fails with `MBEDTLS_ERR_X509_CERT_VERIFY_FAILED`
(`-9984`), and a CA file that is not a certificate with
`MBEDTLS_ERR_X509_INVALID_FORMAT` (`-8576`). The compatibility layer returns
only Mbed TLS codes: a wolfSSL error with no Mbed TLS match returns a general
code for the call, such as `MBEDTLS_ERR_SSL_INTERNAL_ERROR`.

`check.sh` runs the client and servers from `BIN_DIR`, by default the current
directory.

An IP address passed to `-S` fails with `MBEDTLS_ERR_SSL_BAD_INPUT_DATA`
(`-135`), because the compatibility layer does not map IP address checks yet.
