# Description

Simple example of ACERT (Attribute Certificate) support with wolfSSL compat layer,
and OpenSSL.

Supports:

- printing
- signing (openssl only)
- verifying

Based on this example:
- https://github.com/philljj/acert-test

## Prerequisites

Building wolfSSL: Build wolfSSL with

```sh
$ ./configure --enable-acert --enable-opensslextra  --enable-rsapss
$ make
$ make install
```

Building OpenSSL: At the time of writing this README, attribute cert support
requires openssl master. Clone and build openssl with:

```sh
git clone https://github.com/openssl/openssl.git --depth=1 || exit 1
cd openssl || exit 1
./Configure || exit 1
make || exit 1
sudo make install
```

## Building the examples

Build the wolfssl example with:

```sh
make wolfssl_acert
```

Build the openssl example with:

```sh
make wolfssl_acert
```

Note: you may need to use this script to set your environment

```sh
source set_env
info: using env: /usr/local/lib64/:/usr/local/lib/
```
## Examples

### ACERT verification with pubkey

```sh
$./wolfssl_acert -f acerts/acert.pem -k acerts/acert_pubkey.pem
info: using acert file: acerts/acert.pem
info: using pubkey file: acerts/acert_pubkey.pem
info: PEM_read_bio_X509_ACERT: good
info: acert version: 1
info: PEM_read_bio_PUBKEY: good
info: X509_ACERT_verify: good
info: acert_do_test: good
success
```
### Sign and generate ACERT with RSA-PSS with OpenSSL, verify with wolfSSL

1. Use `certs/acert.pem` as input `-f`, and generate new keys and sign `-s`,
   use RSA-PSS `-r`, and write to file `-w`:

```sh
$./openssl_acert -f certs/acert.pem -srw
info: using acert file: certs/acert.pem
info: using rsa_pss
info: using mdname: SHA2-256
info: using mask alg: mgf1 with SHA1
info: PEM_read_bio_X509_ACERT: good
info: acert version: 1
info: X509_ACERT_sign: good
info: wrote acert to file: acert_new.pem
info: wrote pubkey to file: pkey_new.pem
info: X509_ACERT_verify: good
info: acert_do_test: good
success
```

2. Finally, test wolfssl verify using newly generated `acert_new.pem` and
   `pkey_new.pem`:
```sh
$./wolfssl_acert -f acert_new.pem -k pkey_new.pem
info: using acert file: acert_new.pem
info: using pubkey file: pkey_new.pem
info: PEM_read_bio_X509_ACERT: good
info: acert version: 1
info: PEM_read_bio_PUBKEY: good
info: X509_ACERT_verify: good
info: acert_do_test: good
success
```
