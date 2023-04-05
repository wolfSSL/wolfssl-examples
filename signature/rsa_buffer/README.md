# RSA sign and verify example

## Configure, build and install wolfSSL
```
./configure
make
sudo make install
```

**NOTE:** if any build issues due to previous installations please run `ldconfig`

## Build and run example

**NOTE:** The `sign_vfy.sh` script performs the steps below.

```
make sign
./sign "This is the message" > signature.h
make verify
./verify
```

**NOTE:** To Build and run example without Makefile:
```
gcc -o sign sign.c -lwolfssl
./sign <message> > signature.h
gcc -o verify verify.c -lwolfssl
./verify
```

Please contact support@wolfssl.com with any questions or concerns!

Best wishes in all your testing!
- The wolfSSL Team

