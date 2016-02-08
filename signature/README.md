# wolfSSL Signature Example

This directory contains:

A simple example of using wolfSSL to sign and verify binary data. It supports RSA and ECC for signing and MD2, MD4, MD5, SHA, SHA256, SHA384 and SHA512.

This example creates a signature from a hash of the file and signs it using a generated key. Then it performs a verification using the signature and public key.


## Compiling and Running the Example

```
$ make
$ ./firmware [filename] [sig] [hash]
```

## Usage

Usage: signature <filename> <sig> <hash>
  <sig>: 1=ECC (def), 2=RSA, 3=RSA (w/DER Encoding)
  <hash>: 1=MD2, 2=MD4, 3=MD5, 4=SHA, 5=SHA256 (def), 6=SHA384, 7=SHA512, 8=MD5+SHA
