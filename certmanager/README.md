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

The `generate_falcon_chains.sh` script will allow you to use the OQS project's
OpenSSL in order to generate a self-signed CA certificate and entity
certificate that use Falcon. In the OpenSSL directory, run the script to
generate the certificates and then copy into this directory.

```
$ make
$ ./falcon_certverify
```

