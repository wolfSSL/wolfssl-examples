# OCSP Examples

Online Certificate Status Protocol (OCSP) is used for obtaining the revocation status of an X.509 digital certificate.


## OCSP non-blocking example

This uses a google.com certificate chain to demonstrate validating revocation status using an OCSP server.

The Google.com certificate defines OCSP in the X509v3 extension "Authority Information Access" section.

Example usage:

```sh
$ ./configure --enable-ocsp CFLAGS="-DHAVE_IO_TIMEOUT -DWOLFSSL_NONBLOCK_OCSP"
$ make
$ sudo make install

% make
gcc -o ocsp_nonblock ocsp_nonblock.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lwolfssl

% ./ocsp_nonblock
Loaded Trusted CA dir ca_certs (ret 1)
Convert Google.com PEM cert to DER (ret 1)
Verify Google.com cert: 1
OCSP Lookup:
	URL: http://ocsp.pki.goog/gts1c3
	Domain: ocsp.pki.goog
	Path: /gts1c3
	Port: 80
OCSP Response: ret 471, nonblock count 409421
Check OCSP for Google.com (ret 1)
Ret = 1: success
```

## OCSP non-blocking Async example

This uses your system certificate chain to demonstrate validating revocation status using an OCSP
public server with wolfSSL's Asynchronous cryptography support.

**NOTE:** Before building this example the asynchronous support must be obtained from
(https://github.com/wolfSSL/wolfAsyncCrypt) and installed into wolfSSL by following the
instructions in the *README* file contained in the wolfAsyncCrypt repository.

The example uses youtube.com as the public server.

Example usage:

```sh
$ ./configure --enable-ocsp --enable-asynccrypt --enable-sni --enable-alpn --enable-ocspstapling 
              --enable-ocspstapling2 --enable-opensslextra --enable-curve25519 CFLAGS="-DWOLFSSL_NONBLOCK_OCSP"
$ make
$ sudo make install

% make
gcc -o ocsp_nonblock_async ocsp_nonblock_async.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lwolfssl

% ./ocsp_nonblock_async
WolfSSL AsyncCrypt Enabled
WolfSSL AsyncCrypt with Simulation Mode
Connecting...
wolfSSL_connect() returned -1 (error code -108)
wolfSSL_connect() returned -1 (error code -108)
wolfSSL_connect() returned -1 (error code -108)
ocsp_cb(): http://ocsp.pki.goog/gsr1
  simulate 'want read'
wolfSSL_connect() returned -1 (error code -408)
ocsp_cb(): http://ocsp.pki.goog/gsr1
verify_cb()
  preverify_ok = 1
wolfSSL_connect() returned -1 (error code -108)
ocsp_cb(): http://ocsp.pki.goog/gtsr1
  simulate 'want read'
wolfSSL_connect() returned -1 (error code -408)
ocsp_cb(): http://ocsp.pki.goog/gtsr1
verify_cb()
  preverify_ok = 1
wolfSSL_connect() returned -1 (error code -108)
ocsp_cb(): http://ocsp.pki.goog/gts1c3
  simulate 'want read'
wolfSSL_connect() returned -1 (error code -408)
ocsp_cb(): http://ocsp.pki.goog/gts1c3
verify_cb()
  preverify_ok = 1
wolfSSL_connect() returned -1 (error code -108)
CONNECTED
  Closing connection...
wolfSSL_shutdown() failed with code 2 (error 0)
  CLOSED
test_connect() failed
CONNECT FAILED

DONE
```

The example uses `/etc/ssl/certs/ca-certificates.crt` as the system certs file by default. If your
system doesn't have this file, just run the executable with the path to your own cert file.

```
./ocsp_nonblock_asynccrypt ../../mycerts/ca.crt
```
