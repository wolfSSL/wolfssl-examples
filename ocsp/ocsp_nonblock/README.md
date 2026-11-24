# OCSP Examples

Online Certificate Status Protocol (OCSP) is used for obtaining the revocation status of an X.509 digital certificate.


## OCSP non-blocking example

This connects to a public server, fetches its leaf certificate, and checks that
certificate's revocation status with a non-blocking OCSP lookup. The certificate
is fetched live so nothing static can expire.

It uses `www.digicert.com` because that certificate still has an OCSP responder
(many CAs, including Google, have dropped OCSP). The bundled `ca_certs` hold the
DigiCert CAs used to verify it.

Example usage:

```sh
$ ./configure --enable-ocsp --enable-opensslextra --enable-sni \
      CFLAGS="-DHAVE_IO_TIMEOUT -DWOLFSSL_NONBLOCK_OCSP -DKEEP_PEER_CERT \
              -DWOLFSSL_OCSP_SCREEN_RESPONDER"
$ make
$ sudo make install

% make
gcc -o ocsp_nonblock ocsp_nonblock.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lwolfssl

% ./ocsp_nonblock
Fetched www.digicert.com certificate (1777 bytes)
Loaded Trusted CA dir ca_certs (ret 1)
Verify www.digicert.com cert: 1
OCSP Lookup:
	URL: http://ocsp.digicert.com
	Domain: ocsp.digicert.com
	Path: /
	Port: 80
OCSP Response: ret 471, nonblock count 1
Resp ret: 471
Check OCSP for www.digicert.com (ret 1)
Ret = 1: success
```

The `Ret = 1` line is wolfSSL's success code, not the exit status: the program
exits `0` on success and a non-zero error code on failure, so it works in scripts.

If DigiCert's chain changes, pass an alternate CA directory as an argument, e.g.
`./ocsp_nonblock /etc/ssl/certs`.

The certificate fetch (TCP connect plus TLS handshake) and the OCSP responder
connect each time out after 10 seconds. On a slow network, raise it at build
time with `CFLAGS="-DDEFAULT_TIMEOUT_SEC=30 ..."`.

With `WOLFSSL_OCSP_SCREEN_RESPONDER` both clients refuse OCSP responder
addresses in loopback, private and link-local ranges, so a certificate cannot
point the lookup at an internal host. They resolve the responder once and
connect to the address they checked, so a changing DNS answer cannot get
around it. Leave the define out if your responder runs on such an address.

## OCSP non-blocking Async example

This demonstrates validating revocation status using an OCSP public server with wolfSSL's
Asynchronous cryptography support.

The example uses `www.digicert.com`, whose certificate still has an OCSP
responder (many CAs, including Google, have dropped OCSP).

The name lookups, the TCP connects, the TLS handshake and the OCSP lookup
inside it share one 15 second deadline (`HANDSHAKE_TIMEOUT_SEC` in the source).
The OCSP lookup is done with wolfSSL's HTTP helpers on a non-blocking socket so
it can be held to that deadline too. Name lookups use glibc's `getaddrinfo_a()`,
so this client builds on Linux.

Example usage:

```sh
$ ./configure --enable-ocsp --enable-asynccrypt --enable-sni --enable-alpn --enable-ocspstapling 
              --enable-ocspstapling2 --enable-opensslextra \
              CFLAGS="-DWOLFSSL_NONBLOCK_OCSP -DHAVE_IO_TIMEOUT -DWOLFSSL_OCSP_SCREEN_RESPONDER"
$ make
$ sudo make install

% make
gcc -o ocsp_nonblock_async ocsp_nonblock_async.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lwolfssl

% ./ocsp_nonblock_async
WolfSSL AsyncCrypt Enabled
WolfSSL AsyncCrypt with Simulation Mode
Connecting...
verify_cb()
  preverify_ok = 1
ocsp_cb(): http://ocsp.digicert.com
  simulate 'want read'
ocsp_cb(): http://ocsp.digicert.com
verify_cb()
  preverify_ok = 1
CONNECTED
  Closing connection...
  CLOSED
CONNECT PASSED

DONE
```

If you already have wolfSSL installed and do not want to change it, build a
local wolfSSL copy but skip `make install`, then compile the example straight
against that build (this leaves your existing install untouched):

```sh
# In your local wolfSSL source directory, build only (no install):
$ ./configure --enable-ocsp --enable-asynccrypt --enable-sni --enable-alpn \
      --enable-ocspstapling --enable-ocspstapling2 --enable-opensslextra \
      CFLAGS="-DWOLFSSL_NONBLOCK_OCSP -DHAVE_IO_TIMEOUT -DWOLFSSL_OCSP_SCREEN_RESPONDER"
$ make

# Back in this example directory, point the compiler at that build:
$ WOLFSSL=/path/to/wolfssl
$ gcc -o ocsp_nonblock_async ocsp_nonblock_async.c -Wall -Os \
      -I$WOLFSSL -L$WOLFSSL/src/.libs -lwolfssl
$ LD_LIBRARY_PATH=$WOLFSSL/src/.libs ./ocsp_nonblock_async
```

By default it loads the bundled `ca_certs/DigiCert_Global_Root_G2.pem` to check
digicert.com's certificate, so it runs on its own. digicert.com is a real server,
so its certificate can change. If the check fails, pass another cert file, such as
your system CA bundle:

```
./ocsp_nonblock_async /etc/ssl/certs/ca-certificates.crt
```

On success it prints `CONNECTED` and, at the end, `CONNECT PASSED`. The
`verify_cb` and `ocsp_cb` lines show the certificate check and OCSP lookup
running.
