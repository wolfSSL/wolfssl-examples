# Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using RSA

## Building

### Build and install wolfSSL

```
./configure && make && sudo make install
```

### Build example

```
make
```

## Usage

```
./wolfsigtest
...
Signatures match!
```

```
./openssltest

...
CRYPTO: signature verify OK! 1
CRYPTO: EXPECTED signature verify OK! 1

```
