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

Note, some examples require additional features, such as "--with-libz" and 
"--enable-pwdbased". To build wolfSSL with support for all examples, use:

```
$ ./configure --enable-pkcs7 --enable-pwdbased --enable-cryptocb --with-libz CFLAGS="-DWOLFSSL_DER_TO_PEM"
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

Some of the examples which use the CompressedData content type require the
zlib library to be installed. The default Makefile has the linking against
(-lz) commented out. Before making the CompressedData example, please
uncomment the following line in Makefile:

```
#ZLIB    += -lz
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

Most of the examples listed below output a DER-encoded version of the
PKCS#7/CMS bundle that was generated. These DER-encoded files can be used to
do further analysis, or debugging with external tools.  Two helpful tools
for doing this include the `openssl cms` application and the `dumpasn1` app.

Debugging with `dumpasn1` ([dumpasn1 source file](https://www.cs.auckland.ac.nz/~pgut001/dumpasn1.c))

```
$ dumpasn1 encryptedData.der
 0  80: SEQUENCE {
 2   9:   OBJECT IDENTIFIER encryptedData (1 2 840 113549 1 7 6)
13  67:   [0] {
15  65:     SEQUENCE {
17   1:       INTEGER 0
20  60:       SEQUENCE {
22   9:         OBJECT IDENTIFIER data (1 2 840 113549 1 7 1)
33  29:         SEQUENCE {
35   9:           OBJECT IDENTIFIER aes256-CBC (2 16 840 1 101 3 4 1 42)
46  16:           OCTET STRING 08 83 47 90 5D 9F D6 AA DC 25 CE B2 87 9A 10 CF
      :           }
64  16:         [0] 3C 22 EA 61 64 FB 21 30 77 8A CE B0 5A A7 35 DE
      :         }
      :       }
      :     }
      :   }

0 warnings, 0 errors.
```

Debugging with `openssl cms`

```
$ openssl cms -inform der -in envelopedData.der -cmsout -print -noout
CMS_ContentInfo: 
  contentType: pkcs7-encryptedData (1.2.840.113549.1.7.6)
  d.encryptedData: 
    version: <ABSENT>
    encryptedContentInfo: 
      contentType: pkcs7-data (1.2.840.113549.1.7.1)
      contentEncryptionAlgorithm: 
        algorithm: aes-256-cbc (2.16.840.1.101.3.4.1.42)
        parameter: OCTET STRING:
          0000 - 08 83 47 90 5d 9f d6 aa-dc 25 ce b2 87 9a 10   ..G.]....%.....
          000f - cf                                             .
        encryptedContent: 
          0000 - 3c 22 ea 61 64 fb 21 30-77 8a ce b0 5a a7 35   <".ad.!0w...Z.5
          000f - de                                             .
  unprotectedAttrs:
    <EMPTY>
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

### CompressedData

Example file: `compressedData.c`
Generated bundle file: `compressedData.der`

This example creates a PKCS#7/CMS CompressedData bundle. After creating the
bundle, it decodes, uncompresses, and verifies the bundle.

The generated CompressedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./compressedData
Successfully encoded CompressedData bundle (compressedData.der)
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

### EnvelopedData using KEKRI RecipientInfo

Example file: `envelopedData-kekri.c`
Generated bundle file: `envelopedDataKEKRI.der`

This example creates a PKCS#7/CMS EnvelopedData bundle that includes a KEKRI
RecipientInfo type. It uses AES256-CBC as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./envelopedData-kekri
Successfully encoded EnvelopedData bundle (envelopedDataKEKRI.der)
Successfully decoded EnvelopedData bundle (envelopedDataKEKRI.der)
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

### AuthEnvelopedData using KEKRI RecipientInfo

Example file: `authEnvelopedData-kekri.c`
Generated bundle file: `authEnvelopedDataKEKRI.der`

This example creates a PKCS#7/CMS AuthEnvelopedData bundle that includes a KEKRI
RecipientInfo type. It uses AES256-GCM as the encryption algorithm, and
specifies the enveloped content type as DATA. After creating the bundle,
it decodes the enveloped bundle and verifies the operation was successful.

The generated EnvelopedData bundle is written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./authEnvelopedData-kekri
Successfully encoded AuthEnvelopedData bundle (authEnvelopedDataKEKRI.der)
Successfully decoded AuthEnvelopedData bundle (authEnvelopedDataKEKRI.der)
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
content type encapsulates a FirmwarePkgData type. After creating the
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
CompressedData content type encapsulates a FirmwarePkgData type. After creating
the bundles, the app decodes them and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-CompressedFirmwarePkgData
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Compressed FirmwarePkgData (signedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### SignedData using Crypto Callback

Build wolfssl using: `./configure --enable-pkcs7 --enable-pwdbased --enable-cryptocb`.

Example file: `signedData-cryptocb.c`
Generated bundle files: `signedData_cryptocb_noattrs.der`,
                        `signedData_cryptocb_attrs.der`

This example creates a PKCS#7/CMS SignedData bundle using the wolfCrypt
Crypto callback. This allows a user to register a callback to do
cryptographic operations outside of wolfCrypt proper. This can be useful
in order to take advantage of hardware-based cryptography instead of the
default software implementation.

It uses RSA with SHA256 as the signature algorithm, and specifies
the signed content type as DATA. After creating the bundle, the app decodes it
and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-cryptocb
Successfully encoded SignedData bundle (signedData_cryptocb_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded SignedData bundle (signedData_cryptocb_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData with Detached Signature

Example file: `signedData-DetachedSignature.c`
Generated bundle files: `signedData_detached_noattrs.der`,
                        `signedData_detached_attrs.der`

This example creates a PKCS#7/CMS SignedData bundle that does not include
the EncapsulatedContent eContent. This is known as a detached signature, and
is useful when signing a large amount of data to decrease the size of the
CMS bundle. It uses RSA with SHA256 as the signature algorithm, and specifies
the signed content type as DATA. After creating the bundle, the app decodes it
and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-DetachedSignature
Successfully encoded SignedData bundle (signedData_detached_noattrs.der)
Successfully verified SignedData bundle.
Successfully encoded SignedData bundle (signedData_detached_attrs.der)
Successfully verified SignedData bundle.
```

### SignedData encapsulating Encrypted Compressed FirmwarePkgData

Example file: `signedData-EncryptedCompressedFirmwarePkgData.c`
Generated bundle files: `signedEncryptedCompressedFirmwarePkgData_noattrs.der`,
                        `signedEncryptedCompressedFirmwarePkgData_attrs.der`

This example creates two PKCS#7/CMS SignedData bundles, one with attributes and
one without them. It uses RSA with SHA256 as the the signature algorithm,
and specifies the signed content type as CompressedData. The inner
CompressedData content type encapsulates an EncryptedData type, which in turn
encapsulates a FirmwarePkgData type. After creating the bundles, the app
decodes them and verifies the operation was successful.

The generated SignedData bundles are written out to a file for analysis and
additional debugging.

If wolfSSL has been configured and compiled with debug support, the bytes
of the bundle will be printed out to the terminal window.

```
./signedData-EncryptedCompressedFirmwarePkgData
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_noattrs.der)
Successfully extracted and verified bundle contents
Successfully encoded Signed Encrypted Compressed FirmwarePkgData (signedEncryptedCompressedFPD_attrs.der)
Successfully extracted and verified bundle contents
```

### Converting P7B Certificate Bundle to PEM using PKCS7 SignedData API

Build wolfssl using: `./configure --enable-pkcs7 CFLAGS="-DWOLFSSL_DER_TO_PEM"`

Example file: `signedData-p7b.c`

This example parses a .p7b certificate bundle using wolfCrypt's PKCS#7
SignedData API, looping over each extracted certificate, converting each
certificate to PEM format (from DER), and printing it to the terminal for
info/reference.

```
./signedData-p7b
Successfully verified SignedData bundle.
CERT [0] size = 1291 bytes
converted DER to PEM, pemSz = 1805
CERT [0] PEM:
-----BEGIN CERTIFICATE-----
MIIFBzCCA++gAwIBAgIJAPFcmUNmPZYEMA0GCSqGSIb3DQEBCwUAMIGeMQswCQYD
VQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96ZW1hbjEVMBMG
A1UECgwMd29sZlNTTF8yMDQ4MRkwFwYDVQQLDBBQcm9ncmFtbWluZy0yMDQ4MRgw
FgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEWEGluZm9Ad29s
ZnNzbC5jb20wHhcNMjEwMjEwMTk0OTUyWhcNMjMxMTA3MTk0OTUyWjCBnjELMAkG
A1UEBhMCVVMxEDAOBgNVBAgMB01vbnRhbmExEDAOBgNVBAcMB0JvemVtYW4xFTAT
BgNVBAoMDHdvbGZTU0xfMjA0ODEZMBcGA1UECwwQUHJvZ3JhbW1pbmctMjA0ODEY
MBYGA1UEAwwPd3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdv
bGZzc2wuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwwPRK/45
pDJFO1PIhCsqfHSavaoqUgdH1qY2sgcyjtC6aXvGw0Se1IFI/S1oootnu6F1yDYs
StIb94u6zw357+zxgR57mwNHmr9lzH9lJGmm6BSJW+Q098WwFJP1Z3s6enjhAVZW
kaYTQo3SPECcTO/Rht83URsMoTv18aNKNeThzpbfG36/TpfQEOioCDCBryALQxTF
dGe0MoJvjYbCiECZNoO6HkByIhfXUmUkc7DO7xnNrv94bHvAEgPUTnINUG07ozuj
mV6dyNkMhbPZitlUJttt+qy7/yVMxNF59HHThkAYE7BjtXJOMMSXhIYtVi/XFfd/
wK71/Fvl+6G60wIDAQABo4IBRDCCAUAwHQYDVR0OBBYEFDPYRWbXaIcYflQNcCeR
xybXhWXAMIHTBgNVHSMEgcswgciAFDPYRWbXaIcYflQNcCeRxybXhWXAoYGkpIGh
MIGeMQswCQYDVQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96
ZW1hbjEVMBMGA1UECgwMd29sZlNTTF8yMDQ4MRkwFwYDVQQLDBBQcm9ncmFtbWlu
Zy0yMDQ4MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEW
EGluZm9Ad29sZnNzbC5jb22CCQDxXJlDZj2WBDAMBgNVHRMEBTADAQH/MBwGA1Ud
EQQVMBOCC2V4YW1wbGUuY29thwR/AAABMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggr
BgEFBQcDAjANBgkqhkiG9w0BAQsFAAOCAQEAuitI0ajjwoRClqF85fFGukz3h1fH
eMjBMsRp/4W7XWrdyYd+/rv0/RUKTJSVgDCQRQP4M4fKX3Q4pNBax2U4w7Doh7FJ
Mrms6fvTCB2kUXvX2Ut5NaI6C+QMoAKcoWjhXWyOLjok3rvWHKesLs1XREj2cuDH
W5PcfVtkDheEaCyVHSyG1rB0Z1Fue/TVYThRsxjjEBZzSzaKimIF9VaKviHheH2/
rUX5C/WvoGIB/T9J3zk8/0boCv5ca7tBpWTxXJtRTLxtn6Mg7elI4am+CC2FQlnW
Q31HIqX6H6JYdgtwHB1ZHaq+XS0lfLEGtsCqKKqTfNC9Q62RUBx7TfPk1w==
-----END CERTIFICATE-----
```

## Support

Please email wolfSSL support at support@wolfssl.com with any questions about
PKCS#7/CMS support in wolfCrypt, or about issues with these examples.

## References

[RFC 2315 - PKCS #7: Cryptographic Message Syntax Version 1.5](https://tools.ietf.org/html/rfc2315)

[RFC 5652 - Cryptographic Message Syntax (CMS)](https://tools.ietf.org/html/rfc5652)

[wolfSSL Manual](https://www.wolfssl.com/docs/wolfssl-manual/)

[wolfCrypt API Reference](https://www.wolfssl.com/doxygen/wolfcrypt_API.html)

