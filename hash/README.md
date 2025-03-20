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

This example shows how to hash an input file using SHA-256.

```
./sha256-hash input.txt
Hash input file input.txt
Hash result is: 75294625788129796c09fcbf313ea16e2883356e322adc2f956b37dbdc10b6a7
```

### `sha512-hash`

This example shows how to hash an input file using SHA-512.

```
./sha512-hash input.txt
Hash input file input.txt
Hash result is: ead56209da2dfb3562263aadc57d9382f0f7cb579ebb6dbf2f20bfd3cb68aaaad422f6ce6f1a88ec6c326edcf8456f650579b6e20eb39f3bb444bee8b65615ed
```

### `sha3-256-hash`

This example shows how to hash an input file using SHA3-256.

```
./sha3-256-hash input.txt
Hash input file input.txt
Hash result is: 0704c6ca55e7e5c706b543f07da1daed8149c838549096df6a52dac5f95f2fe0
```

### `hash-file`

This example shows how to hash an input file using hash wrapper API.

```
./hash-file SHA256 input.txt
Hash input file input.txt
Hash result is: 75294625788129796c09fcbf313ea16e2883356e322adc2f956b37dbdc10b6a7

./hash-file SHA512 input.txt
Hash input file input.txt
Hash result is: ead56209da2dfb3562263aadc57d9382f0f7cb579ebb6dbf2f20bfd3cb68aaaad422f6ce6f1a88ec6c326edcf8456f650579b6e20eb39f3bb444bee8b65615ed

./hash-file SHA3-256 input.txt
Hash input file input.txt
Hash result is: 0704c6ca55e7e5c706b543f07da1daed8149c838549096df6a52dac5f95f2fe0
```

### `sha256-hash-string`

This example shows how to hash a string using SHA256.

```
./sha256-hash-string
String to hash: 'String to hash'
Hash result is: d4476d30fd94c746eb38d8a1b3931aa81d1e485be5a6362f47598017a91cb5d2
```

### `sha256-hash-oneshot-string`

This example shows how to hash a string using wolfSSL's SHA-256 oneshot API.

```
./sha256-hash-oneshot-string
String to hash: 'String to hash'
Hash result is: d4476d30fd94c746eb38d8a1b3931aa81d1e485be5a6362f47598017a91cb5d2
```

### `sha3-256-hash-oneshot-string`

This example shows how to hash a string using wolfSSL's SHA3-256 oneshot API.

```
./sha3-256-hash-oneshot-string
String to hash: 'String to hash'
Hash result is: 10d69e59ac10b1d81755733f323bdadca3e04e4b17df72f5b343d6da701a4225
```

## Support

For questions please email us at support@wolfssl.com.
