# wolfSSL Signature Example

This directory contains:

A simple example of using wolfSSL to sign and verify binary data using SHA512 and ECC256.

This example creates a signature from a hash of the file and signs it using a generated ECC key. Then it performs a verification using the signature and public key. 


## Compiling and Running the Example

```
$ make
$ ./firmware [filename]
```
