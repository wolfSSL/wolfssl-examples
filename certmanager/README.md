# wolfSSL CertManager Examples

This directory contains examples of using the wolfSSL CertManager to verify
certificates in a standalone manner, separate from an SSL/TLS connection.

## Examples

- **certverify.c** - Basic certificate verification example
- **certloadverifybuffer.c** - Certificate verification from memory buffer
- **certverify_ocsp.c** - Certificate verification with OCSP
  revocation checking

## Building wolfSSL

```bash
cd wolfssl
./autogen.sh
./configure --enable-crl --enable-ocsp
make all
make check
sudo make install
```

## Building the Examples

```bash
cd wolfssl-examples/certmanager
make all
```

## Running the Basic Example

```bash
./certverify
./certloadverifybuffer
```

## Running the OCSP Example

The `certverify_ocsp` example demonstrates certificate verification with
OCSP (Online Certificate Status Protocol) checking.

The OCSP demo requires an OCSP responder to be running. You can start one using
OpenSSL:

**Terminal 1 - Start the OCSP Responder:**

From the wolfSSL library source code directory:

```bash
openssl ocsp -port 22221 -ndays 365 \
    -index certs/ocsp/index-intermediate1-ca-issued-certs.txt \
    -rsigner certs/ocsp/ocsp-responder-cert.pem \
    -rkey certs/ocsp/ocsp-responder-key.pem \
    -CA certs/ocsp/intermediate1-ca-cert.pem \
    -text
```

**Terminal 2 - Run the Example:**
```bash
./certverify_crl_ocsp
```

### Expected Output

When the OCSP responder is running, you should see output that indicates basic
verification passed and OCSP checking is done and successful.  Importantly,
OpenSSL should also show some pretty prints of the OCSP request.

### OCSP Certificate Structure

The OCSP test uses the following certificate chain from the wolfSSL repository's
`certs/ocsp/` directory. This assumes the wolfssl and wolfssl-examples
repositories are in the same parent directory.


- **root-ca-cert.pem** - Root CA certificate
- **intermediate1-ca-cert.pem** - Intermediate CA that issued the server cert
- **server1-cert.pem** - Server certificate with OCSP URL in AIA extension
- **ocsp-responder-cert.pem** - Certificate for the OCSP responder
- **ocsp-responder-key.pem** - Private key for the OCSP responder

The server1-cert.pem certificate contains an Authority Information Access (AIA)
extension pointing to `http://127.0.0.1:22221` for OCSP queries.

