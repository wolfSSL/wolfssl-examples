# wolfSSL X509 Field Extraction Example

Example of parsing a DER encoded self-signed certificate and extracting
public key and subject name information.

## Compiling and Running the Example

To compile, first build wolfSSL with the OpenSSL compatibility layer enabled:

```
$ cd wolfssl-X.X.X
$ ./configure --enable-opensslextra
$ make
$ sudo make install
```

Then, compile the example app:

```
$ make
```

Next, run the example app with a cert of your choice:

EXAMPLE A:

```
$ ./app ../../certs/ca-cert.der RSA
PUBLIC KEY:
3082010A0282010100BF0CCA2D14B21E84425BCD381F4AF24D7510F1B6359FDFCA7D0398D3ACDE0366EE2AF1D8B07D6E07540B1098214D80CB1220E7CC4FDE457DC9727732EACA90BB695210032FA8F395C5F18B62561BEF676FA4104195AD0A9BE3A5C0B0D2707650305BA8E8082C7CEDA7A27A8D38291CACC7EDF27C95B095827D495C38CD7725EFBD807553943C3DCA635B9F15B5D31D132F19D13CDB763ACCB87DC9E5C2D7DA406FD821DC731B422D539CFE1AFC7DAB7A363F98DE847C0567CE6A143887A9F18CB568CB687F71202BF5A063F5562FA326D2B76FB15A17D7389908FE93586FFEC3134908160BA74D6700523167234E98ED51451DB904D90BECD828B34BBDED36790203010001
SIG TYPE = 655
CN = www.wolfssl.com (15)
COUNTRY = US (2)
LOCALITY = Bozeman (7)
STATE = Montana (7)
ORG = Sawtooth (8)
ORG UNIT = Consulting (10)
```


EXAMPLE B: (Assuming you've built and run the certgen_example example from
            wolfssl-examples/certgen/ directory with WOLFSSL_ALT_NAMES)

NOTE: Must configure with `--enable-opensslall` to see the IP address output!

```
$ ./app ../../certgen/newCert.der ECC
PUBLIC KEY:
3059301306072A8648CE3D020106082A8648CE3D03010703420004DC0E533A07160404DDA2D28685F8AB10880A1C17556443BE71C113BF5888268866187D976CB444CDE848C3AA6802251BF1FD2582FF1285BE869F5592ADD60C5D
SIG TYPE = 524
CN = www.yourDomain.com (18)
COUNTRY = US (2)
LOCALITY = Bozeman (7)
STATE = MT (2)
ORG = yourOrgNameHere (15)
ORG UNIT = yourUnitNameHere (16)
DNS:localhost
DNS:example.com
DNS:127.0.0.1
IP:127.0.0.1
```

For support, please contact support@wolfssl.com
