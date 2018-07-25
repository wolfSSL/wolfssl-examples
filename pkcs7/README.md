# Examples demonstrating PKCS7

## Building

### Build and install wolfSSL

```
./configure --enable-pkcs7 && make && sudo make install
```

### Build Example

```
make
gcc -o pkcs7 pkcs7.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o ecc-verify ecc-verify.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
```

### Debug

To enable debug change the Makefile to:

```
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
```

Build wolfSSL using: `./configure --enable-pkcs7 --enable-debug && make && sudo make install`


## Usage


### `pkcs7-verify`

```
./pkcs7-verify 
Der 1633
PKCS7 Verify Success
```
