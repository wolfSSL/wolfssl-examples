# Asymmetric Ed448 Examples

The build option used for wolfSSL are:

```
./configure --enable-ed448 --enable-keygen
make
sudo make install
sudo ldconfig
```


## curve448_keys

The `curve448_keys.c` example shows how to work with storing and loading keys after they have been generated.

1. Creates a key structure
2. Stores the private key in DER format.
3. Loads DER private key back into a curve448_key struct.

## ed448_sign

The `curve448_sign.c:` example takes a random message and private key, creates a signature then verifies it.

## ed448_verify

The `curve448_verify.c` example uses NIST test vectors to demonstrate hashing a message and verifying an Ed448 signature.

## ed448_pub

The `curve448_pub` example code shows how to extracting an Ed448 public key from private key.

