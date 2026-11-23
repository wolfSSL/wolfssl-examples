# Mbed TLS Compatibility Example

`client-tls-mbedtls.c` is an application written for the Mbed TLS 4.x API,
built on wolfSSL through the Mbed TLS compatibility layer in
[wolfPSA](https://github.com/wolfSSL/wolfPSA). Its `tls_get()` function is the
client from the "Porting a TLS Client" chapter of the Mbed TLS to wolfSSL
migration guide, plus retrying partial writes, `poll()` before non-blocking
retries and an error when the server closes without a reply.

wolfPSA provides the PSA Crypto API on wolfCrypt, as TF-PSA-Crypto does for
Mbed TLS. Built with `MBEDTLS_COMPAT=1`, it also provides the Mbed TLS TLS,
X.509 and network API on wolfSSL, so the client links `libwolfpsa.a` in place
of `libmbedtls`, `libmbedx509` and `libtfpsacrypto`. The calls mapped so far
and the known differences from Mbed TLS are in wolfPSA's README, section
"Mbed TLS compatibility".

## Building

Build wolfPSA with the compatibility layer, next to `wolfssl-examples`. It
compiles wolfCrypt and the wolfSSL TLS sources from `WOLFSSL_PATH` into
`libwolfpsa.a` with its own settings, so the client needs no wolfSSL install.
`WOLFSSL_PATH` must be a checkout of wolfSSL's master branch:

```
cd wolfPSA
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

Start a TLS 1.3 server, run the client, then stop the server:

```
make check-server-tls13
./check-server-tls13 &
sleep 1
./client-tls-mbedtls 127.0.0.1
kill %1
```

```
Handshake done, TLSv1.3, TLS1-3-AES-256-GCM-SHA384
tls_get() returned 0
```

The host is a name or an IPv4 or IPv6 address. The client checks the server
certificate against `../certs/ca-cert.pem` and the host. Options:

* `-A <file>` CA certificate file, PEM or DER
* `-S <name>` server name to send and check, instead of the host
* `-p <port>` server port, default 11111
* `-N` use a non-blocking socket

To connect to a public HTTPS server, give the port and the system's CA
bundle, whose path depends on the system:

```
./client-tls-mbedtls -p 443 -A /etc/ssl/certs/ca-certificates.crt \
    www.wolfssl.com
```

```
Handshake done, TLSv1.3, TLS1-3-AES-128-GCM-SHA256
tls_get() returned 0
```

When the certificate check fails, the client prints the reasons from
`mbedtls_x509_crt_verify_info()`, as the Mbed TLS sample programs do. For
example, with a CA that did not sign the server's chain:

```
./client-tls-mbedtls -p 443 -A ../certs/ca-cert.pem www.wolfssl.com
```

```
  ! The certificate is not correctly signed by the trusted CA
tls_get() returned -9984
```

## Testing

`make check` builds `../tls/server-tls.c` as `check-server-tls12` and
`check-server-tls13` and runs the client against them for TLS 1.2 and 1.3,
blocking and non-blocking, with an IP address and a DNS name as the server
name and a host name to connect to, and for each failure it should report:
wrong server name or IP address, wrong CA, missing CA file and a CA file that
is not a certificate. Each handshake must also print the protocol line, and
each failed certificate check at least one reason. `../tls/server-tls13` is
not used because it requires a client certificate, which this client does not
send.

A wrong server name, IP address or CA fails with
`MBEDTLS_ERR_X509_CERT_VERIFY_FAILED` (`-9984`), a missing CA file with
`MBEDTLS_ERR_PK_FILE_IO_ERROR` (`-15872`), and a CA file that is not a
certificate with `MBEDTLS_ERR_X509_INVALID_FORMAT` (`-8576`), as in Mbed TLS.
The compatibility layer returns only Mbed TLS codes: a wolfSSL error with no
Mbed TLS match returns a general code for the call, such as
`MBEDTLS_ERR_SSL_INTERNAL_ERROR`.

`check.sh` runs the client and servers from `BIN_DIR`, by default the current
directory.
