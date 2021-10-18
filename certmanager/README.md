# wolfSSL CertManager Example

This directory contains:

A simple example of using the wolfSSL CertManager to verify a certificate
in a standalone manner, separate from an SSL/TLS connection.

## Compiling and Running the Example

```
$ make
$ ./certverify
```
## Verification of OQS Falcon Certificates

Please see wolfssl/INSTALL for instructions on how to build and install the
Open Quantum Safe project's liboqs. Once you have built that, you will then
need to build the Open Quantum Safe project's OpenSSL. Instructions for
downloading and building their OpenSSL fork can be found here:

https://github.com/open-quantum-safe/openssl/releases/tag/OQS-OpenSSL_1_1_1-stable-snapshot-2021-08

Note that installation of the OpenSSL fork is NOT neccessary.a

The `generate_falcon_chains.sh` script will allow you to use the OQS project's
OpenSSL in order to generate a self-signed CA certificate and entity
certificate that uses Falcon. In the OpenSSL directory, run the script to
generate the certificates and then copy them into this directory.

Once that is complete, compile and run `falcon_certverify`:

```
$ make
$ ./falcon_certverify
```

