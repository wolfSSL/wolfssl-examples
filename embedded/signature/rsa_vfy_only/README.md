# RSA Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using RSA

First, set the path to wolfssl directory to variable WOLFROOT in the Makefile.

## Building

### Build example

```
make
```

### Usage
```
./verify
```

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
built with the option `math=sp arch=arm64`
```
./bench
---------------------------------------------------------------
Enabled WOLFSSL_SP_ARM64
---------------------------------------------------------------
Running benchmark...
Please Wait 3.00 seconds
Takes 3.00 Sec for 237053 times,    79017.45 Cycles/sec
Finished Benchmark
```


built with the option `math=tfm`
```
./bench
---------------------------------------------------------------
Enabled TFM
---------------------------------------------------------------
Running benchmark...
Please Wait 3.00 seconds
Takes 3.00 Sec for 76438 times,    25479.23 Cycles/sec
Finished Benchmark
```

# Tracking memory
To see a stack and heap memory usage

```
make mem
```
## Example Output
```
./verify
Verified
total   Allocs   =         0
total   Deallocs =         0
total   Bytes    =         0
peak    Bytes    =         0
current Bytes    =         0
stack used = 11384
```


Best wishes in all your testing!

- The wolfSSL Team

