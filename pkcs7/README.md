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

Example file: `envelopedData-ktri.c`

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

### AuthEnvelopedData using KTRI RecipientInfo

Example file: `authEnvelopedData-ktri.c`

```
./authEnvelopedData-ktri
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataKTRI.der)
```

### AuthEnvelopedData using KARI RecipientInfo

Example file: `authEnvelopedData-kari.c`

```
./authEnvelopedData-kari
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataKARI.der)
```

### AuthEnvelopedData using PWRI RecipientInfo

Example file: `authEnvelopedData-pwri.c`

```
./authEnvelopedData-pwri
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataPWRI.der)
```

### AuthEnvelopedData using ORI RecipientInfo

Example file: `authEnvelopedData-ori.c`

```
./authEnvelopedData-ori
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataORI.der)
```

### SignedData

Example file: `signedData.c`

```
./signedData
Successfully encoded SignedData bundle (signedData_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded SignedData bundle (signedData_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData encapsulating FirmwarePkgData

Example file: `signedData-FirmwarePkgData.c`

```
./signedData-FirmwarePkgData
Successfully encoded Signed FirmwarePkgData bundle (signedFirmwarePkgData_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded Signed FirmwarePkgData bundle (signedFirmwarePkgData_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData encapsulating Encrypted FirmwarePkgData

Example file: `signedData-EncryptedFirmwarePkgData.c`

```
./signedData-EncryptedFirmwarePkgData
Successfully encoded Signed Encrypted FirmwarePkgData (signedEncryptedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Encrypted FirmwarePkgData (signedEncryptedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### SignedData encapsulating Compressed FirmwarePkgData

Example file: `signedData-CompressedFirmwarePkgData.c`

```
./signedData-CommpressedFirmwarePkgData
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### SignedData encapsulating Encrypted Compressed FirmwarePkgData

Example file: `signedData-EncryptedCompressedFirmwarePkgData.c`

```
./signedData-EncryptedCommpressedFirmwarePkgData
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

