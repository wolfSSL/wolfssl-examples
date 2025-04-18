# X9.146 Examples

This README file explains how to setup various demos for showing our X9.146
features in action. X9.146 is a specification of a certificate format that
allows for dual public keys and signatures in a single certificate.

Traditionally, there are only public key, signature algorithm specifier and
signature value. These are known as the native elements. The X9.146 scheme also
allows for additional alternative public key, signature algorithm specifier and
signature value as optional X.509 certificate extensions.

The X9.146 specification also specifies how to use these certificates in TLS
1.3. In the ClientHello message, a CKS extension is added. This extension
specifies the ability and preference for which signature(s) is/are sent in the
CertificateVerify message. The presence of the value specifies ability; the
order of the values specifies preference. The following values are defined: 

- NATIVE 0x01
- ALTERNATIVE 0x02
- BOTH 0x03
- EXTERNAL 0x04 (not supported)

The ServerHello message would have the extension and it would only have a single
value which would be one of the ones in the list sent over by the client.  That
is going to specify what is sent in the CertificateVerify message. BOTH is simply the concatenation of the native and alternative signatures; native first.

## Post-Quantum

Tested with these wolfSSL build options for MLDSA certificates:

```sh
./autogen.sh  # If cloned from GitHub
./configure --enable-experimental  --enable-dual-alg-certs --enable-dilithium --enable-debug
make
sudo make install
sudo ldconfig # required on some targets
```

And need to setup wolfCLU:

```sh
./autogen.sh  # Cloned from GitHub
./configure
make
sudo make install
sudo ldconfig # required on some targets
```

In the directory where this README.md file is found, clean up previous build
products and certificates and then build the applications.

```sh
make clean all
```
NOTE: `clean` removes certificates and keys in this directory.

### What to Expect

There will be a lot of debug output going to stderr. On the client side, during
the call to `DoTls13Certificate()`, please search for the following messages to
confirm that the alternative signature was verified:

```
Alt signature has been verified!
Verified Peer's cert
```

These debug messages indicate that the client has verified the alternative
post-quantum certificate chain. The second message indicates that normal
verification was also successful.

On the client side, during the call to `DoTls13CertificateVerify()` look for
messages that indicate both conventional and post-quantum verification:

For example, if you are doing ECDSA with MLDSA, you will see the following:

```
Doing ECC peer cert verify
wolfSSL Entering EccVerify
wolfSSL Leaving EccVerify, return 0
Doing MLDSA peer cert verify
wolfSSL Leaving DoTls13CertificateVerify, return 0
```

### ECDSA Demos

#### P-256 and MLDSA44 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
# CA
wolfssl genkey ecc -name secp256r1 -out ca-key -outform pem -output keypair

wolfssl pkey -in ca-key.priv -inform pem -out ca-key.der -outform der

wolfssl pkey -pubin -in ca-key.pub -inform pem -pubout -out ca-pubkey.der -outform der

mv ca-key.priv ca-key.pem

# Server
wolfssl genkey ecc -name secp256r1 -out server-key -outform pem -output keypair

wolfssl pkey -in server-key.priv -inform pem -out server-key.der -outform der

wolfssl pkey -in server-key.priv -inform pem -pubout -out server-pubkey.der -outform der
```

Generate the certificate chain:

```sh
./gen_ecdsa_mldsa_dual_keysig_root_cert 2

./gen_ecdsa_mldsa_dual_keysig_server_cert 2
```

Convert the DER encoded resulting certificates and keys into PEM:

```sh
wolfssl x509 -in ca-cert-pq.der -inform der -out ca-P256-mldsa44-cert.pem -outform pem

wolfssl x509 -in server-cert-pq.der -inform der -out server-P256-mldsa44-cert.pem -outform pem

mv server-key.priv server-P256-key.pem

cp ../certs/mldsa44_server_key.pem server-mldsa44-key-pq.pem
```
Then in wolfssl's source directory:

```sh
examples/server/server -d -v 4 -c ../wolfssl-examples/X9.146/server-P256-mldsa44-cert.pem -k ../wolfssl-examples/X9.146/server-P256-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-mldsa44-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P256-mldsa44-cert.pem
```

#### P-384 and MLDSA65 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
# CA
wolfssl genkey ecc -name secp384r1 -out ca-key -outform pem -output keypair

wolfssl pkey -in ca-key.priv -inform pem -out ca-key.der -outform der

wolfssl pkey -pubin -in ca-key.pub -inform pem -pubout -out ca-pubkey.der -outform der

mv ca-key.priv ca-key.pem

# Server
wolfssl genkey ecc -name secp384r1 -out server-key -outform pem -output keypair

wolfssl pkey -in server-key.priv -inform pem -out server-key.der -outform der

wolfssl pkey -in server-key.priv -inform pem -pubout -out server-pubkey.der -outform der
```

Generate the certificate chain:

```sh
./gen_ecdsa_mldsa_dual_keysig_root_cert 3

./gen_ecdsa_mldsa_dual_keysig_server_cert 3
```

Convert the DER encoded resulting certificates and keys into PEM:

```sh
wolfssl x509 -in ca-cert-pq.der -inform der -out ca-P384-mldsa65-cert.pem -outform pem

wolfssl x509  -in server-cert-pq.der -inform der -out server-P384-mldsa65-cert.pem -outform pem

mv server-key.priv server-P384-key.pem

cp ../certs/mldsa65_server_key.pem server-mldsa65-key-pq.pem
```

Then in wolfssl's source directory:

```sh
examples/server/server -d -v 4 -c ../wolfssl-examples/X9.146/server-P384-mldsa65-cert.pem -k ../wolfssl-examples/X9.146/server-P384-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-mldsa65-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P384-mldsa65-cert.pem
```

#### P-521 and MLDSA87 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
# CA
wolfssl genkey ecc -name secp521r1 -out ca-key -outform pem -output priv

wolfssl pkey -in ca-key.priv -inform pem -out ca-key.der -outform der

wolfssl pkey -in ca-key.priv -inform pem -pubout -out ca-pubkey.der -outform der

mv ca-key.priv ca-key.pem

# Server
wolfssl genkey ecc -name secp521r1 -out server-key -outform pem -output priv

wolfssl pkey -in server-key.priv -inform pem -out server-key.der -outform der

wolfssl pkey -in server-key.priv -inform pem -pubout -out server-pubkey.der -outform der
```

Generate the certificate chain:

```sh
./gen_ecdsa_mldsa_dual_keysig_root_cert 5

./gen_ecdsa_mldsa_dual_keysig_server_cert 5
```

Convert the DER encoded resulting certificates and keys into PEM:

```sh
wolfssl x509 -in ca-cert-pq.der -inform der -out ca-P521-mldsa87-cert.pem -outform pem

wolfssl x509  -in server-cert-pq.der -inform der -out server-P521-mldsa87-cert.pem -outform pem

mv server-key.priv server-P521-key.pem

cp ../certs/mldsa87_server_key.pem server-mldsa87-key-pq.pem
```

Then in wolfssl's source directory:

```sh
examples/server/server -d -v 4 -c ../wolfssl-examples/X9.146/server-P521-mldsa87-cert.pem -k ../wolfssl-examples/X9.146/server-P521-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-mldsa87-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P521-mldsa87-cert.pem
```

### RSA Demos

#### RSA-3072 and MLDSA44 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
# CA
wolfssl -genkey rsa -size 3072 -out ca-key -outform der -output priv

mv ca-key.priv ca-key.der

# Server
wolfssl -genkey rsa -size 3072 -out server-key -outform der -output priv

mv server-key.priv server-key.der
```

Generate the certificate chain:

```sh
./gen_rsa_mldsa_dual_keysig_root_cert

./gen_rsa_mldsa_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```sh
wolfssl x509 -in ca-cert-pq.der -inform der -out ca-rsa3072-mldsa44-cert.pem -outform pem

wolfssl x509  -in server-cert-pq.der -inform der -out server-rsa3072-mldsa44-cert.pem -outform pem

wolfssl pkey  -in server-key.der -inform der -out server-rsa3072-key.pem -outform pem

cp ../certs/mldsa44_server_key.pem server-mldsa44-key-pq.pem
```
Then in wolfssl's source directory:

```sh
examples/server/server -d -v 4 -c ../wolfssl-examples/X9.146/server-rsa3072-mldsa44-cert.pem -k ../wolfssl-examples/X9.146/server-rsa3072-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-mldsa44-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-rsa3072-mldsa44-cert.pem
```

## Generating a Certificate Chain and Adding Alternative keys and Signatures

In the directory where this README.md file is found, build the applications:

```sh
make all
```

Generate the various keys:
```sh
openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out ca-key.der -outform der

openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out server-key.der -outform der

openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out alt-ca-key.der -outform der

openssl pkey -in alt-ca-key.der -inform der -pubout -out alt-ca-pub-key.der -outform der

openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out alt-server-key.der -outform der

openssl pkey -in alt-server-key.der -inform der -pubout -out alt-server-pub-key.der -outform der
```

Generate the certificate chain:

```sh
./gen_dual_keysig_root_cert

./gen_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```sh
openssl x509  -in ./ca-cert.der -inform der -out ca-cert.pem -outform pem

openssl x509  -in ./server-cert.der -inform der -out server-cert.pem -outform pem

openssl pkey  -in ./server-key.der -inform der -out server-key.pem -outform pem

openssl pkey  -in ./alt-server-key.der -inform der -out alt-server-key.pem -outform pem

```

Note: These will not work with the TLS 1.3 demo.
