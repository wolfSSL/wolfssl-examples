# wolfCrypt Test Demo Application

This demo application runs the wolfCrypt test app. It tests all
algorithms enabled in wolfCrypt. It has been tested using a Raspberry Pi
and SE050 EdgeLock development kit.

## Building the Demo

Before building this demo, follow initial setup instructions in the parent
[README.md](../../README.md).

**IMPORTANT:** This example requires the test.c and test.h files from a
wolfSSL source package be copied into this directory before compiling and
running the example. There are stub files included in this example directory
that should be overwritten:

```
$ cp wolfssl-X.X.X/wolfcrypt/test/test.c ./
$ cp wolfssl-X.X.X/wolfcrypt/test/test.h ./
```

Once this example directory has been copied into the SE05x middleware directory
tree in the correct location, compiling the middleware will also compile this
demo application:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ cmake --build .
```

## Running the Demo

To run the demo:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c/bin
$ ./wolfcrypt_test
```

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfcrypt_test
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
sss   :WARN :Communication channel is Plain.
sss   :WARN :!!!Not recommended for production use.!!!
App   :INFO :running setconfig
App   :INFO :Ran setconfig successfully
------------------------------------------------------------------------------
 wolfSSL version 5.5.1
------------------------------------------------------------------------------
error    test passed!
MEMORY   test passed!
base64   test passed!
asn      test passed!
RANDOM   test passed!
MD5      test passed!
SHA      test passed!
SHA-256  test passed!
SHA-384  test passed!
SHA-512  test passed!
Hash     test passed!
HMAC-MD5 test passed!
HMAC-SHA test passed!
HMAC-SHA256 test passed!
HMAC-SHA384 test passed!
HMAC-SHA512 test passed!
HMAC-KDF    test passed!
TLSv1.3 KDF test passed!
GMAC     test passed!
Chacha   test passed!
POLY1305 test passed!
ChaCha20-Poly1305 AEAD test passed!
AES      test passed!
AES192   test passed!
AES256   test passed!
AES-GCM  test passed!
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id FFFF
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10000
RSA      test passed!
DH       test passed!
PWDBASED test passed!
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10001
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10002
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10003
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10004
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10005
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10006
CHRIS: before ecc_sign_hash call, ret = 0
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10007
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10008
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10009
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000A
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000B
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000C
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000D
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000E
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1000F
CHRIS: before ecc_sign_hash call, ret = 0
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10010
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10011
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10012
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10013
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10014
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10015
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10016
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10017
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10018
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10019
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001A
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001B
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001C
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001D
CHRIS: before ecc_sign_hash call, ret = 0
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001E
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1001F
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10020
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10021
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10022
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10023
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10024
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10025
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10026
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10027
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10028
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10029
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002A
CHRIS: before ecc_sign_hash call, ret = 0
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002B
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002C
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002D
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002E
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 1002F
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10030
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10031
ECC      test passed!
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10032
ECC buffer test passed!
logging  test passed!
time test passed!
mutex    test passed!
memcb    test passed!
Test complete
App   :INFO :Ran wolfCrypt test
App   :INFO :ex_sss Finished
```

## Demo Notes

The `test.c` and `test.h` file in this directory have been copied
directly from the wolfSSL download directory, at:

```
wolfssl-X.X.X/wolfcrypt/test/test.c
wolfssl-X.X.X/wolfcrypt/test/test.h
```

It is expected to see warning messages in the console log about failures to
delete key ids. wolfCrypt internally tries to delete temporary/test keys,
and if those keys have not been stored in the SE050 the debug logs will show
these messages.

