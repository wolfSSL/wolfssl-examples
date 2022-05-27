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
