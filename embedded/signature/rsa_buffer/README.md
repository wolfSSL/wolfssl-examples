Configure and install wolfSSL with these options:

./configure
make
make install

(if any build issues due to previous installations please run 'ldconfig`)

To compile without Makefile:

gcc -o sign sign.c -lwolfssl

./sign \<message> > signature.h
gcc -o verify verify.c -lwolfssl


To sign a message:
./sign \<message>

To verify the signature with the message:

./verify


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
Running benchmark...
Please Wait 3.00 seconds
Takes 3.00 Sec for 236782 times,    78927.31 Cycles/sec
Finished Benchmark
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
./verify
Verified
total   Allocs   =         0
total   Deallocs =         0
total   Bytes    =         0
peak    Bytes    =         0
current Bytes    =         0
stack used = 41872
```


Best wishes in all your testing!

- The wolfSSL Team

