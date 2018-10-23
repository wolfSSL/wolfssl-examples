# Examples demonstrating PKCS7

## Building

### Build and Install wolfSSL

```
$ ./configure --enable-pkcs7
$ make
$ sudo make install
```

### Build Examples

Running `make` will try to compile all C source files into their own
respective example application. Each source file can be compiled separately
if desired, for example:

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


## Examples Description and Usage


### pkcs7-verify

```
./pkcs7-verify 
Der 1633
PKCS7 Verify Success
```

### EncryptedData

Example file: `encryptedData.c`

```
./encryptedData
Successfully encoded EncryptedData bundle (encryptedData.der)
```

### EnvelopedData using KTRI RecipientInfo

Example file: `envelopedData-ktri.c

```
./envelopedData-ktri 
Successfully encoded EnvelopedData bundle (envelopedDataKTRI.der)
```

### EnvelopedData using KARI RecipientInfo

Example file: `envelopedData-kari.c`

```
./envelopedData-kari
Successfully encoded EnvelopedData bundle (envelopedDataKARI.der)
```

### EnvelopedData using PWRI RecipientInfo

Example file: `envelopedData-pwri.c`

```
./envelopedData-pwri
Successfully encoded EnvelopedData bundle (envelopedDataPWRI.der)
```

### EnvelopedData using ORI RecipientInfo

Example file: `envelopedData-ori.c`

```
./envelopedData-ori
Successfully encoded EnvelopedData bundle (envelopedDataORI.der)
```

