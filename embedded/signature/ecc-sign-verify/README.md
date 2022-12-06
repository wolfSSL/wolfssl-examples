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

  

# Signature verification Benchmark

You can run benchmark program to compare the speed of signature verification between TFM and SP
### SP
Faster math library

If you build on x86_64 system:
```
make sp_x86_64
```
else if Aarch64 system: 
```
make sp_arm64 
``` 
### TFM
```
make tfm
```
then a benchmark program is generated.

## Example Output
when running on Aarch64 Mac book Air M2 (2022) built with the option `sp_arm64`
```
./benchmark
---------------------------------------------------------------
Enabled WOLFSSL_SP_ARM64
---------------------------------------------------------------
Running ECC Sign Verify Benchmarks...
ECC Key Size 112       1275.78 Cycles/sec
ECC Key Size 128       1351.68 Cycles/sec
ECC Key Size 160       1368.65 Cycles/sec
ECC Key Size 192       1382.20 Cycles/sec
ECC Key Size 224       1385.06 Cycles/sec
ECC Key Size 239       1401.38 Cycles/sec
ECC Key Size 256      12830.67 Cycles/sec
ECC Key Size 320        626.52 Cycles/sec
ECC Key Size 384        634.85 Cycles/sec
ECC Key Size 512        279.71 Cycles/sec
ECC Key Size 521        279.15 Cycles/sec
```

# Debugging memory
To see a stack and heap memory consumption

Configure and install wolfSSL with these options first:

```
./configure --enable-trackmemory && make && sudo make install
```

After returning to this directory
```
make debug_memory
```
## Example Output
```
./ecc_sign_verify
Key size is 112, byteField = 14
Successfully verified signature w/ ecc key size 112!
Key size is 128, byteField = 16
Successfully verified signature w/ ecc key size 128!
Key size is 160, byteField = 20
Successfully verified signature w/ ecc key size 160!
Key size is 192, byteField = 24
Successfully verified signature w/ ecc key size 192!
Key size is 224, byteField = 28
Successfully verified signature w/ ecc key size 224!
Key size is 239, byteField = 30
Successfully verified signature w/ ecc key size 239!
Key size is 256, byteField = 32
Successfully verified signature w/ ecc key size 256!
Key size is 320, byteField = 40
Successfully verified signature w/ ecc key size 320!
Key size is 384, byteField = 48
Successfully verified signature w/ ecc key size 384!
Key size is 512, byteField = 64
Successfully verified signature w/ ecc key size 512!
Key size is 521, byteField = 66
Successfully verified signature w/ ecc key size 521!

total   Allocs   =       572
total   Deallocs =       572
total   Bytes    =    247247
peak    Bytes    =      5161
current Bytes    =         0
stack used = 9248
```


Best wishes in all your testing!

- The wolfSSL Team
