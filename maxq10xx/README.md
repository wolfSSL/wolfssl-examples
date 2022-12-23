# wolfSSL with Analog Devices MAXQ10xx

This example implements a very simple client application that uses the Analog
Devices MAXQ1065 or MAXQ1080 to do cryptographic operations. Please see the
product documentation for what operations are supported.

NOTE: These instructions are for a MAXQ1065 or MAXQ1080 evaluation board plugged
      into the 40-pin GPIO headers of a RaspberryPi. The SDK and example
      application are built and executed on the RaspberryPi.

## Building and Installing wolfSSL

You need to have wolfSSL built via the MAXQ10xx SDK.  Please contact Analog
Devices to request the SDK. Make sure you have all the required hardware and
software. Follow the instructions to build the SDK. Once completed, there will
be two instances of wolfSSL in the SDK directory; one for server operations
(`wolfssl`) and one for client operations (`maxq10xx-wolfssl`). The client
instance's cryptographic operations are performed by the MAXQ1065 or MAXQ1080.
Enter the `maxq10xx-wolfssl` and install the client instance:

```
cd /path/to/maxq10xx-sdk/maxq10xx-wolfssl
sudo make install
sudo ldconfig
```

This will put the appropriate header files and dynamic libraries in
`/usr/local/include` and `/usr/local/lib/`.

NOTE: Do NOT install the instance for server operations (`wolfssl`).

## Setting Up the MAXQ1065 or MAXQ1080

Follow the SDK instructions for generating and loading the desired cryptographic
artifacts into MAXQ1065 or MAXQ1080. This will depend on the TLS version and
algorithms you want to use.

## Dummy Keys

The build of wolfSSL uses our pkcallbacks configuration to allow MAXQ1065 or
MAXQ1080 to do the cryptographic operations. In order for wolfSSL to understand
the algorithms being used, on the command line we substitute the private key
with a dummy public key at runtime. You can generate these dummy public keys by
running the following commands:

```
openssl x509 -in <ecc_cert>.pem -pubkey -noout > ecc-p256-pub.pem
openssl x509 -in <rsa_cert>.pem -pubkey -noout > rsa-2048-pub.pem
```

`<ecc_cert>.pem` must be a certificate with an ECC P-256 public key in it.
`<rsa_cert>.pem` must be a certificate with an RSA 2048-bit public key in it.
For your convenience, they have already been provided.

```
make maxq10xx-wolfssl-client
```

## Running the Example

The client and server are executed with different command-line parameters
depending on the desired algorithms and TLS version. First, go into the correct
locations in your shell:

```
cd /path/to/maxq10xx-sdk/wolfssl
```

```
cd /path/to/wolfssl-examples/maxq10xx
```

Depending on which of the following algorithms and TLS versions, execute the
associated commands as shown.

### TLS 1.2 PSK (MAXQ1065 or MAXQ1080)

```
./examples/server/server -s -v 3 -l PSK-AES128-CCM-8
```

```
./maxq10xx-wolfssl-client -tls12 -psk
```

### TLS 1.2 ECC (MAXQ1065 or MAXQ1080)

```
./examples/server/server -F -v 3 -l ECDHE-ECDSA-AES128-GCM-SHA256 \
   -c ../pki/CA_secp256r1/cert_server_ECDSA_secp256r1_secp256r1.pem \
   -k ../pki/CA_secp256r1/privkey_server_ECDSA_secp256r1_secp256r1.pem \
   -A ../pki/CA_secp256r1/cert_CA.pem
```

```
./maxq10xx-wolfssl-client -tls12 -ecc
```

### TLS 1.3 PSK (Only MAXQ1080)

```
./examples/server/server -v 4 -s -l TLS13-AES128-GCM-SHA256
```

```
./maxq10xx-wolfssl-client -tls13 -psk
```

### TLS 1.3 ECC (Only MAXQ1080)

```
./examples/server/server -F -v 4 -l TLS13-AES128-GCM-SHA256 \
    -c ../pki/CA_secp256r1/cert_server_ECDSA_secp256r1_secp256r1.pem \
    -k ../pki/CA_secp256r1/privkey_server_ECDSA_secp256r1_secp256r1.pem \
    -A ../pki/CA_secp256r1/cert_CA.pem
```

```
./maxq10xx-wolfssl-client -tls13 -ecc
```

### TLS 1.3 RSA and FFDHE (Only MAXQ1080)

```
./examples/server/server -F -v 4 -l TLS13-AES128-GCM-SHA256 \
    -c ../pki/CA_RSA_2048/cert_server_RSA_2048_RSA_2048.pem \
    -k ../pki/CA_RSA_2048/privkey_server_RSA_2048_RSA_2048.pem \
    -A ../pki/CA_RSA_2048/cert_CA.pem
```

```
./maxq10xx-wolfssl-client -tls13 -rsa
```

### Expected Output

The server will print very little status information about the algorithms and
ciphersuite negotiated. It will wait for a message from the client.

The client will give lots of debug and status logging. It will then send the
message "Hello from MAXQ10xx!".

The server will print out the message it received and send the message "I hear
you fa shizzle!" and then terminate.

The client will print the message it received from the server along with more
debug and status logging and then terminate.

