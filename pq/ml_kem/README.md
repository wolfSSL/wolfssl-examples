# Description

Simple example of wolfCrypt ML-KEM keygen, encapsulation, and decapsulation.

Requires wolfSSL is built with:

```sh
$ ./autogen.sh
$ ./configure --enable-mlkem
$ make
$ sudo make install
$ sudo ldconfig
```

Build the ML-KEM example:

```sh
$ gcc -o ml_kem ml_kem.c -I/usr/local/include -L/usr/local/lib -lwolfssl
```

# Usage

```
./ml_kem
```

Alice will generate a public key and private key pair. The operation is known as
key generation.

Bob will use only Alice's public key to generate a shared secret and an
ecapsulated instance of the shared secret; this is known as the ciphertext. The
operation is known as encapsulation.

Alice will use Bob's ciphertext and her private key to obtain the same shared
secret. The operation is known as decapsulation.

# Credit

The wolfSSL Team would like to thank Professor Bill Buchanan of Edinburgh Napier
University for initially creating this example.
