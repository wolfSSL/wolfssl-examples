# Asymmetric ECC Examples

The build option used for wolfSSL are:

```
./configure --enable-ecc CFLAGS="-DWOLFSSL_PUBLIC_MP"
make
sudo make install
sudo ldconfig
```


## ecc_keys

The `ecc_keys.c` example shows how to work with storing and loading keys after they have been generated.

1. Creates a key structure
2. Stores the private key in DER format.
3. Loads DER private key back into a ecc_key struct.

## ecc_sign

The `ecc_sign.c:` example takes a random message and private key, creates a signature then verifies it.

## ecc_verify

The `ecc_verify.c` example uses NIST test vectors to demonstrate hashing a message and verifying an ECC signature.

## ecc_pub

The `ecc_pub` example code shows how to extracting an ECC public key from private key.
