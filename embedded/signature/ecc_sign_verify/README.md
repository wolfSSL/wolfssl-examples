# Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using ECC

First, set the path to wolfssl directory to variable WOLFROOT in the Makefile.
## Building

### Build example

```
make
```

## Usage

```
./ecc_sign_verify
Key size is 224, byteField = 28
Successfully verified signature w/ ecc key size 224 (SECP224R1)!
Key size is 239, byteField = 30
Successfully verified signature w/ ecc key size 239 (PRIME239V1)!
Key size is 256, byteField = 32
Successfully verified signature w/ ecc key size 256 (SECP256R1)!
Key size is 384, byteField = 48
Successfully verified signature w/ ecc key size 384 (SECP384R1)!
Key size is 521, byteField = 66
Successfully verified signature w/ ecc key size 521 (SECP521R1)!
```

NOTE: Also an option to dump out the signatures. For more verbose output
      uncomment define in example "SHOW_SIGS_IN_EXAMPLE"

  

# Signature verification Benchmark

You can generate benchmark program to compare the speed of signature verification between TFM and SP
### SP
Faster math library

If you build for x86_64 system:
```
make bench math=sp arch=x64
```
else if Aarch64 system: 
```
make bench math=sp arch=arm64
``` 
then a benchmark program is generated.
### TFM

```
make bench math=tfm
```
NOTE: When using TFM, No Architecture specification is required.

## Example Output
- built with the option `math=sp arch=arm64`
```
./ecc_sign_verify_bench
---------------------------------------------------------------
Enabled WOLFSSL_SP_ARM64
---------------------------------------------------------------
Running ECC Sign Verify Benchmarks...
ECC Key Size 224       1385.06 Cycles/sec (SECP224R1)
ECC Key Size 239       1401.38 Cycles/sec (PRIME239V1)
ECC Key Size 256      12830.67 Cycles/sec (SECP256R1)
ECC Key Size 384        634.85 Cycles/sec (SECP384R1)
ECC Key Size 521        279.15 Cycles/sec (SECP521R1)
```

# Tracking memory
To see a stack and heap memory usage.

```
make mem
```
## Example Output
```
./ecc_sign_verify_mem
Key size is 224, byteField = 28
Successfully verified signature w/ ecc key size 224 (SECP224R1)!
Key size is 239, byteField = 30
Successfully verified signature w/ ecc key size 239 (PRIME239V1)!
Key size is 256, byteField = 32
Successfully verified signature w/ ecc key size 256 (SECP256R1)!
Key size is 384, byteField = 48
Successfully verified signature w/ ecc key size 384 (SECP384R1)!
Key size is 521, byteField = 66
Successfully verified signature w/ ecc key size 521 (SECP521R1)!

total   Allocs   =       170
total   Deallocs =       170
total   Bytes    =     79385
peak    Bytes    =      5629
current Bytes    =         0
stack used = 18288
```


Best wishes in all your testing!

- The wolfSSL Team
