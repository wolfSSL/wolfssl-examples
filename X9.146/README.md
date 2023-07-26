# X9.146 Examples

TODO: explain the standard and add links for reference. 


## Post-Quantum

Tested with these wolfSSL build options:

```sh
./autogen.sh  # If cloned from GitHub
./configure --enable-x9-146 --with-liboqs --enable-debug
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
Alternative signature has been verified!
Verified Peer's cert
```

These debug messages indicate that the client has verified the alternative
post-quantum certificate chain. The second message indicates that normal
verification was also successful.

On the client side, during the call to `DoTls13CertificateVerify()` look for
messages that indicate both conventional and post-quantum verification:

For example, if you are doing ECDSA with Falcon, you will see the following:

```
Doing ECC peer cert verify
wolfSSL Entering EccVerify
wolfSSL Leaving EccVerify, return 0
Doing Falcon peer cert verify
wolfSSL Leaving DoTls13CertificateVerify, return 0
```

### ECDSA Demos

#### P-256 and Dilithium Level 2 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out ca-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out server-key.der -outform der
```

Generate the certificate chain:

```
$ ./gen_ecdsa_dilithium2_dual_keysig_root_cert

$ ./gen_ecdsa_dilithium2_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-P256-dilithium2-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-P256-dilithium2-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-P256-key.pem -outform pem

$ openssl pkey  -in ../certs/dilithium_level2_server_key.der -inform der -out server-dilithium2-key-pq.pem -outform pem
i
(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-P256-dilithium2-cert.pem -k ../wolfssl-examples/X9.146/server-P256-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-dilithium2-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P256-dilithium2-cert.pem
```

#### P-384 and Dilithium Level 3 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-384 -out ca-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-384 -out server-key.der -outform der
```

Generate the certificate chain:

```

$ ./gen_ecdsa_dilithium3_dual_keysig_root_cert

$ ./gen_ecdsa_dilithium3_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-P384-dilithium3-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-P384-dilithium3-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-P384-key.pem -outform pem

$ openssl pkey  -in ../certs/dilithium_level3_server_key.der -inform der -out server-dilithium3-key-pq.pem -outform pem

(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-P384-dilithium3-cert.pem -k ../wolfssl-examples/X9.146/server-P384-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-dilithium3-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P384-dilithium3-cert.pem
```

#### P-521 and Dilithium Level 5 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-521 -out ca-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-521 -out server-key.der -outform der
```

Generate the certificate chain:

```

$ ./gen_ecdsa_dilithium5_dual_keysig_root_cert

$ ./gen_ecdsa_dilithium5_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-P521-dilithium5-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-P521-dilithium5-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-P521-key.pem -outform pem

$ openssl pkey  -in ../certs/dilithium_level5_server_key.der -inform der -out server-dilithium5-key-pq.pem -outform pem
i
(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-P521-dilithium5-cert.pem -k ../wolfssl-examples/X9.146/server-P521-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-dilithium5-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P521-dilithium5-cert.pem
```

#### P-256 and Falcon Level 1 Demo

Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out ca-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out server-key.der -outform der
```

Generate the certificate chain:

```
$ ./gen_ecdsa_falcon1_dual_keysig_root_cert

$ ./gen_ecdsa_falcon1_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-P256-falcon1-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-P256-falcon1-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-P256-key.pem -outform pem

$ openssl pkey  -in ../certs/falcon_level1_server_key.der -inform der -out server-falcon1-key-pq.pem -outform pem
i
(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-P256-falcon1-cert.pem -k ../wolfssl-examples/X9.146/server-P256-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-falcon1-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P256-falcon1-cert.pem
```

#### P-521 and Falcon Level 5 Demo

Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-521 -out ca-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-521 -out server-key.der -outform der
```

Generate the certificate chain:

```

$ ./gen_ecdsa_falcon5_dual_keysig_root_cert

$ ./gen_ecdsa_falcon5_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-P521-falcon5-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-P521-falcon5-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-P521-key.pem -outform pem

$ openssl pkey  -in ../certs/falcon_level5_server_key.der -inform der -out server-falcon5-key-pq.pem -outform pem

(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-P521-falcon5-cert.pem -k ../wolfssl-examples/X9.146/server-P521-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-falcon5-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-P521-falcon5-cert.pem
```

### RSA Demos

#### RSA-3072 and Dilithium Level 2 Demo
Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out ca-key.der -outform der

$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out server-key.der -outform der
```

Generate the certificate chain:

```

$ ./gen_dilithium2_dual_keysig_root_cert

$ ./gen_dilithium2_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-rsa3072-dilithium2-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-rsa3072-dilithium2-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-rsa3072-key.pem -outform pem

$ openssl pkey  -in ../certs/dilithium_level2_server_key.der -inform der -out server-dilithium2-key-pq.pem -outform pem

(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-rsa3072-dilithium2-cert.pem -k ../wolfssl-examples/X9.146/server-rsa3072-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-dilithium2-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-rsa3072-dilithium2-cert.pem
```

#### RSA-3072 and Falcon Level 1 Demo

Generate the various conventional keys; the post-quantum key are pre-generated:

```sh
$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out ca-key.der -outform der

$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out server-key.der -outform der
```

Generate the certificate chain:

```
$ ./gen_falcon1_dual_keysig_root_cert

$ ./gen_falcon1_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ca-cert-pq.der -inform der -out ca-rsa3072-falcon1-cert.pem -outform pem

$ openssl x509  -in server-cert-pq.der -inform der -out server-rsa3072-falcon1-cert.pem -outform pem

$ openssl pkey  -in server-key.der -inform der -out server-rsa3072-key.pem -outform pem

$ openssl pkey  -in ../certs/falcon_level1_server_key.der -inform der -out server-falcon1-key-pq.pem -outform pem

(last one must be done with OQS's openssl fork)
```
Then in wolfssl's source directory:

```
examples/server/server -v 4 -c ../wolfssl-examples/X9.146/server-rsa3072-falcon1-cert.pem -k ../wolfssl-examples/X9.146/server-rsa3072-key.pem --altPrivKey ../wolfssl-examples/X9.146/server-falcon1-key-pq.pem

examples/client/client -v 4 -A ../wolfssl-examples/X9.146/ca-rsa3072-falcon1-cert.pem
```

## Generating a Certificate Chain and Adding Alternative keys and Signatures

Tested with these wolfSSL build options:

```sh
./autogen.sh  # If cloned from GitHub
./configure --enable-x9-146 --enable-debug
make
sudo make install
sudo ldconfig # required on some targets
```

In the directory where this README.md file is found, build the applications:

```sh
make all
```

Generate the various keys:
```sh
$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out ca-key.der -outform der

$ openssl genpkey -algorithm rsa  -pkeyopt rsa_keygen_bits:3072 -out server-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out alt-ca-key.der -outform der

$ openssl pkey -in alt-ca-key.der -inform der -pubout -out alt-ca-pub-key.der -outform der

$ openssl genpkey -algorithm ec -pkeyopt ec_paramgen_curve:P-256 -out alt-server-key.der -outform der

$ openssl pkey -in alt-server-key.der -inform der -pubout -out alt-server-pub-key.der -outform der
```

Generate the certificate chain:

```
$ ./gen_dual_keysig_root_cert

$ ./gen_dual_keysig_server_cert
```

Convert the DER encoded resulting certificates and keys into PEM:

```
$ openssl x509  -in ./ca-cert.der -inform der -out ca-cert.pem -outform pem

$ openssl x509  -in ./server-cert.der -inform der -out server-cert.pem -outform pem

$ openssl pkey  -in ./server-key.der -inform der -out server-key.pem -outform pem

$ openssl pkey  -in ./alt-server-key.der -inform der -out alt-server-key.pem -outform pem

```

Note: These will not work with the TLS 1.3 demo.
