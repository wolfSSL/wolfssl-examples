# wolfSSL Signature Example

This directory contains:

A simple example of using wolfSSL to sign and verify binary data. It supports RSA and ECC for signing and MD2, MD4, MD5, SHA, SHA224, SHA256, SHA384 and SHA512.

This example creates a signature from a hash of the file and signs it using a generated key. Then it performs a verification using the signature and public key.


## Compiling and Running the Example

```
$ make
$ ./signature [filename] [sig] [hash]
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
   the .der formatted files output from application "gen_key_files.c"

3. App "sign_and_verify.c" to use the "test_keys.h" header file buffers for
   importing the public and private keys. App will then sign a msg with the
   private key and verify that signature using the public key

------------------ UPDATE -----------------
November 13 2018:

Added rsa_buffer directory

rsa_buffer directory contains:

1. App "sign.c" to generate a signature using a DER encoded private key into a
   buffer. Prints out a hex encoding of signature.

2. App "verify.c" to verify a signature using a DER encoded public key in and
   binary signature in buffers.

3. Script sign_vfy.sh builds sign, creates a signature with "sign", builds
   verify with message and signature generated and verifies the signature with
   "verify".

