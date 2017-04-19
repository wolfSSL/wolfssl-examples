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

------------------ UPDATE -----------------
April 11 2017:

Added ED25519 directory

ED25519 directory contains:

1. App "gen_key_files.c" to generate public/private keys and output keys to .der
formatted files.

2. genkeybuffers.pl - a perl script to write the header file "test_keys.h" using
   the .der formatted files output from applicaton "gen_key_files.c"

3. App "sign_and_verify.c" to use the "test_keys.h" header file buffers for
   importing the public and private keys. App will then sign a msg with the
   private key and verify that signature using the public key


