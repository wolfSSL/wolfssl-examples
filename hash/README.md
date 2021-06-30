# Examples demonstrating Hash

## Building

### Build and install wolfSSL

```
./configure && make && sudo make install
```

### Build Example

```
make
gcc -fsanitize=address -o sha256-hash sha256-hash.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
```

### Debug

To enable debug change the Makefile to:

```
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
```

Build wolfSSL adding `--enable-debug` and `--disable-shared` to the ./configure.

## Library

To enable using the static library change the Makefile to:

```
LIBS+=$(STATIC_LIB)
#LIBS+=$(DYN_LIB)
```

## Usage

### `sha256-hash`

This example shows how to hash an input file using sha256.

```
./sha256 input.txt
Hash input file input.txt
Hash result is: c279d0c6e7308e9401d3e5ff217fd03af404b3fe6b2e5028aa5138714e85b599
```

## Support

For questions please email us at support@wolfssl.com.
