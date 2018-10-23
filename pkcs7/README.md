# PKCS7/CMS Examples

This directory contains example applications that demonstrate usage of the
wolfCrypt PKCS#7/CMS API, included in the [wolfSSL embedded SSL/TLS library](https://www.wolfssl.com).
It includes examples of several different content types (EncryptedData,
SignedData, EnvelopedData, CompressedData), and demonstrates both
signing/encrypting and verifying/decrypting operations.

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

### Debugging

To enable debug messages change the Makefile to:

```
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
```

Build and install wolfSSL using:

```
./configure --enable-pkcs7 --enable-debug
make
sudo make install
```

## Examples Description and Usage

### pkcs7-verify

```
./pkcs7-verify 
Der 1633
PKCS7 Verify Success
```

### EncryptedData

Example file: `encryptedData.c`
Generated bundle file: `encryptedData.der`

This example creates a PKCS#7/CMS EncryptedData bundle, using AES256-CBC
as the encryption algorithm. After creating the bundle, it decodes the encrypted
bundle and verifies the decryption operation was successful.

The generated EncryptedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./encryptedData
Successfully encoded EncryptedData bundle (encryptedData.der)
```

### EnvelopedData using KTRI RecipientInfo

Example file: `envelopedData-ktri.c`
Generated bundle file: `envelopedDataKTRI.der`

This example creates a PKCS#7/CMS EnvelopedData bundle that includes a KTRI
RecipientInfo type. It uses AES256-CBC as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./envelopedData-ktri
Successfully encoded EnvelopedData bundle (envelopedDataKTRI.der)
```

### EnvelopedData using KARI RecipientInfo

Example file: `envelopedData-kari.c`
Generated bundle file: `envelopedDataKARI.der`

This example creates a PKCS#7/CMS EnvelopedData bundle that includes a KARI
RecipientInfo type. It uses AES256-CBC as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./envelopedData-kari
Successfully encoded EnvelopedData bundle (envelopedDataKARI.der)
```

### EnvelopedData using PWRI RecipientInfo

Example file: `envelopedData-pwri.c`
Generated bundle file: `envelopedDataPWRI.der`

This example creates a PKCS#7/CMS EnvelopedData bundle that includes a PWRI
RecipientInfo type. It uses AES256-CBC as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./envelopedData-pwri
Successfully encoded EnvelopedData bundle (envelopedDataPWRI.der)
```

### EnvelopedData using ORI RecipientInfo

Example file: `envelopedData-ori.c`
Generated bundle file: `envelopedDataORI.der`

This example creates a PKCS#7/CMS EnvelopedData bundle that includes a ORI
RecipientInfo type. It uses AES256-CBC as the encryption algorithm, and
specifies the enveloped content type as DATA. ORI is losely defined by the CMS
RFC and allows users to specify their own encoding/decoding algorithm for
the RecipientInfo. This example uses a simple bitwise complement operation
for the encryption operation. This is not recommended to be used in production
and is simply an example of how user callbacks may be registered and used with
the ORI type. After creating the bundle, it decodes the enveloped bundle and
verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./envelopedData-ori
Successfully encoded EnvelopedData bundle (envelopedDataORI.der)
```

### AuthEnvelopedData using KTRI RecipientInfo

Example file: `authEnvelopedData-ktri.c`
Generated bundle file: `authEnvelopedDataKTRI.der`

This example creates a PKCS#7/CMS AuthEnvelopedData bundle that includes a KTRI
RecipientInfo type. It uses AES256-GCM as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./authEnvelopedData-ktri
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataKTRI.der)
```

### AuthEnvelopedData using KARI RecipientInfo

Example file: `authEnvelopedData-kari.c`
Generated bundle file: `authEnvelopedDataKARI.der`

This example creates a PKCS#7/CMS AuthEnvelopedData bundle that includes a KARI
RecipientInfo type. It uses AES256-GCM as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./authEnvelopedData-kari
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataKARI.der)
```

### AuthEnvelopedData using PWRI RecipientInfo

Example file: `authEnvelopedData-pwri.c`
Generated bundle file: `authEnvelopedDataPWRI.der`

This example creates a PKCS#7/CMS AuthEnvelopedData bundle that includes a PWRI
RecipientInfo type. It uses AES256-GCM as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./authEnvelopedData-pwri
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataPWRI.der)
```

### AuthEnvelopedData using ORI RecipientInfo

Example file: `authEnvelopedData-ori.c`
Generated bundle file: `authEnvelopedDataORI.der`

This example creates a PKCS#7/CMS AuthEnvelopedData bundle that includes a ORI
RecipientInfo type. It uses AES256-GCM as the encryption algorithm, and
specifies the enveloped content type as DATA. ORI is losely defined by the CMS
RFC and allows users to specify their own encoding/decoding algorithm for
the RecipientInfo. This example uses a simple bitwise complement operation
for the encryption operation. This is not recommended to be used in production
and is simply an example of how user callbacks may be registered and used with
the ORI type. After creating the bundle, it decodes the enveloped bundle and
verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./authEnvelopedData-ori
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataORI.der)
```

### SignedData

Example file: `signedData.c`
Generated bundle files: `signedData_noattrs.der`, `signedData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm, and
specifies the signed content type as DATA. After creating the bundle, it
decodes the bundle and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData
Successfully encoded SignedData bundle (signedData_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded SignedData bundle (signedData_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData encapsulating FirmwarePkgData

Example file: `signedData-FirmwarePkgData.c`
Generated bundle files: `signedFirmwarePkgData_noattrs.der`,
                        `signedFirmwarePkgData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm,
and specifies the signed content type as FirmwarePkgData. After creating the
bundle, it decodes the bundle and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-FirmwarePkgData
Successfully encoded Signed FirmwarePkgData bundle (signedFirmwarePkgData_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded Signed FirmwarePkgData bundle (signedFirmwarePkgData_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData encapsulating Encrypted FirmwarePkgData

Example file: `signedData-EncryptedFirmwarePkgData.c`
Generated bundle files: `signedEncryptedFirmwarePkgData_noattrs.der`,
                        `signedEncryptedFirmwarePkgData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm,
and specifies the signed content type as EncryptedData. The inner EncryptedData
content type encpasulates a FirmwarePkgData type. After creating the
bundles, the app decodes them and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-EncryptedFirmwarePkgData
Successfully encoded Signed Encrypted FirmwarePkgData (signedEncryptedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Encrypted FirmwarePkgData (signedEncryptedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### SignedData encapsulating Compressed FirmwarePkgData

Example file: `signedData-CompressedFirmwarePkgData.c`
Generated bundle files: `signedCompressedFirmwarePkgData_noattrs.der`,
                        `signedCompressedFirmwarePkgData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm,
and specifies the signed content type as CompressedData. The inner
CompressedData content type encpasulates a FirmwarePkgData type. After creating
the bundles, the app decodes them and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-CommpressedFirmwarePkgData
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### SignedData encapsulating Encrypted Compressed FirmwarePkgData

Example file: `signedData-EncryptedCompressedFirmwarePkgData.c`
Generated bundle files: `signedEncryptedCompressedFirmwarePkgData_noattrs.der`,
                        `signedEncryptedCompressedFirmwarePkgData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm,
and specifies the signed content type as CompressedData. The inner
CompressedData content type encpasulates an EncryptedData type, which in turn
encapsulates a FirmwarePkgData type. After creating the bundles, the app
decodes them and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-EncryptedCommpressedFirmwarePkgData
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

## Support

Please email wolfSSL support at support@wolfssl.com with any questions about
PKCS#7/CMS support in wolfCrypt, or about issues with these examples.

## References

[RFC 2315 - PKCS #7: Cryptographic Message Syntax Version 1.5](https://tools.ietf.org/html/rfc2315)

[RFC 5652 - Cryptographic Message Syntax (CMS)](https://tools.ietf.org/html/rfc5652)

[wolfSSL Manual](https://www.wolfssl.com/docs/wolfssl-manual/)

[wolfCrypt API Reference](https://www.wolfssl.com/doxygen/wolfcrypt_API.html)

