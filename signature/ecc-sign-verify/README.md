# Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using ECC

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
./ecc_sign_verify
Key size is 112, byteField = 14, maxSigSz = 44
Successfully verified signature w/ ecc key size 112!
Key size is 128, byteField = 16, maxSigSz = 48
Successfully verified signature w/ ecc key size 128!
Key size is 160, byteField = 20, maxSigSz = 56
Successfully verified signature w/ ecc key size 160!
Key size is 192, byteField = 24, maxSigSz = 64
Successfully verified signature w/ ecc key size 192!
Key size is 224, byteField = 28, maxSigSz = 72
Successfully verified signature w/ ecc key size 224!
Key size is 239, byteField = 36, maxSigSz = 88
Successfully verified signature w/ ecc key size 239!
Key size is 256, byteField = 32, maxSigSz = 80
Successfully verified signature w/ ecc key size 256!
Key size is 320, byteField = 40, maxSigSz = 96
Successfully verified signature w/ ecc key size 320!
Key size is 384, byteField = 48, maxSigSz = 112
Successfully verified signature w/ ecc key size 384!
Key size is 512, byteField = 64, maxSigSz = 144
Successfully verified signature w/ ecc key size 512!
Key size is 521, byteField = 66, maxSigSz = 148
Successfully verified signature w/ ecc key size 521!
```

NOTE: Also an option to dump out the signatures. For more verbose output
      uncomment define in example "SHOW_SIGS_IN_EXAMPLE"
