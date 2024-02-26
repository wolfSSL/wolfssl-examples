# RSA Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using RSA

First, set the path to wolfssl directory to variable WOLFROOT in Makefile.

## Building

### Build example

```
make
```

### Usage
```
./verify
```

```
./sign <message>
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
- built with the option `math=sp arch=arm64`
```
./verify_bench
---------------------------------------------------------------
Enabled WOLFSSL_SP_ARM64
---------------------------------------------------------------
Running benchmark...
Please Wait 3.00 seconds
Takes 3.00 Sec for 236782 times,    78927.31 Cycles/sec
Finished Benchmark
```


- built with the option `math=tfm`
```
./verify_bench
---------------------------------------------------------------
Enabled TFM
---------------------------------------------------------------
Running benchmark...
Please Wait 3.00 seconds
Takes 3.00 Sec for 76860 times,    25619.98 Cycles/sec
Finished Benchmark
```

# Tracking memory
To see a stack and heap memory usage

```
make mem
```
## Example Output
```
./verify_mem
Verified
total   Allocs   =         0
total   Deallocs =         0
total   Bytes    =         0
peak    Bytes    =         0
current Bytes    =         0
stack used = 12392
```


Best wishes in all your testing!

- The wolfSSL Team

