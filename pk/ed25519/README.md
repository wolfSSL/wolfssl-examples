# Asymmetric Ed25519 Examples

The build option used for wolfSSL are:

```
./configure --enable-ed25519 --enable-keygen
make
sudo make install
sudo ldconfig
```


## ed25519_keys

The `ed25519_keys.c` example shows how to work with storing and loading keys after they have been generated.

1. Creates a key structure
2. Stores the private key in DER format.
3. Loads DER private key back into a ed25519_key struct.

## ed25519_sign

The `ed25519_sign.c:` example takes a random message and private key, creates a signature then verifies it.

## ed25519_verify

The `ed25519_verify.c` example uses NIST test vectors to demonstrate hashing a message and verifying an Ed25519 signature.

## ed25519_pub

The `ed25519_pub` example code shows how to extracting an Ed25519 public key from private key.
