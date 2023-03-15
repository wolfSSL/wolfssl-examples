# RSA Verify-only example

## Configure, build and install wolfSSL
```
./configure --disable-asn --disable-filesystem \
    --enable-cryptonly --enable-sp=smallrsa2048 --enable-sp-math \
    --disable-dh --disable-ecc --disable-sha224 --enable-rsavfy \
    CFLAGS="-DWOLFSSL_PUBLIC_MP"
make
sudo make install
```

**NOTE:** If any build issues due to previous installations please run 'ldconfig`

## Build and run example
```
make
./verify
```


**NOTE:** To compile without Makefile:
```
gcc -Os -o verify verify.c /usr/local/lib/libwolfssl.a
./verify
```

Please contact support@wolfssl.com with any questions for concerns!

Best wishes in all your testing!

- The wolfSSL Team

