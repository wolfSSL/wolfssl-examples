# RSA Non-blocking Examples

## Design

Enables RsaKey support for non-blocking exptmod operations by setting a non-blocking context using a new API `wc_RsaSetNonBlock`. If the operation is not complete it will return `FP_WOULDBLOCK` and requires being called again until success `0` or failure `<0`.

Example code is based on wolfCrypt test `/wolfcrypt/test/test.c` `rsa_nb_test` function.

## Implementation

* Adds RSA non-blocking support enabled with `WC_RSA_NONBLOCK`.
* Adds new `wc_RsaSetNonBlock` function for enabling / non-block context.
* Adds wolfCrypt test function `rsa_nb_test` to validate.
* Adds RSA blocking time support using `WC_RSA_NONBLOCK_TIME` and `wc_RsaSetNonBlockTime`.

## Code Added in GitHub pull requests:

* 1901: https://github.com/wolfSSL/wolfssl/pull/1901
* 1991: https://github.com/wolfSSL/wolfssl/pull/1991


## Example RSA Non-block

This breaks RSA operations into smallest possible chunks of work.

### Building wolfSSL

```
./autogen.sh
./configure --enable-fastmath CFLAGS="-DWC_RSA_NONBLOCK"
make
sudo make install
```

### Building Example

```
make
gcc -o rsa-nb rsa-nb.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
```

### Example Output

```
./rsa-nb
File ../../certs/client-key.der is 1192 bytes
RSA non-block sign: 8200 times
RSA non-block verify: 264 times
```


## Example RSA Non-block with Time

This adds logic to estimate blocking time based on CPU speed and desired milliseconds to block.

### Building wolfSSL

```
./autogen.sh
./configure --enable-fastmath CFLAGS="-DWC_RSA_NONBLOCK -DWC_RSA_NONBLOCK_TIME"
make
sudo make install
```

### Building Example

```
make
gcc -o rsa-nb rsa-nb.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
```

### Example Output

```
./rsa-nb
File ../../certs/client-key.der is 1192 bytes
CPU Speed is 3100000000, blocking ms 1
RSA non-block sign: 1 times
RSA non-block verify: 1 times
```


## Debugging


1. Build wolfSSL adding `--enable-debug --disable-shared` to the ./configure and do a `sudo make install`:

```
./configure --enable-fastmath CFLAGS="-DWC_RSA_NONBLOCK -DWC_RSA_NONBLOCK_TIME" --enable-debug --disable-shared
make
sudo make install
```

2. Edit Makefile and edit lines 15 - 18 to be:

```
# Options
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
LIBS+=$(STATIC_LIB) -ldl -lm
#LIBS+=$(DYN_LIB)
```

```
make
gcc -o rsa-nb rsa-nb.c -Wall -I/usr/local/include -g -DDEBUG -L/usr/local/lib -lm /usr/local/lib/libwolfssl.a -ldl -lm
gdb ./rsa-nb
run
```

## Support

For questions please email us at support@wolfssl.com.
