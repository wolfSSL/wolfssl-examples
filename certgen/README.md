# Certificate Generation and Signing examples

To test the certgen or csr_example example(s) configure wolfssl with
`./configure --enable-certgen --enable-certreq` or add the defines:

```
#define WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_GEN
```

To test the csr_w_ed25519_example configure wolfssl with:
`./configure --enable-certgen --enable-certreq --enable-ed25519 --enable-keygen`
or add the defines:

```
#define WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_GEN
#define HAVE_ED25519
#define WOLFSSL_KEY_GEN
```

To build use `make`. To cleanup use `make clean`.

If having issues building please check comments in the Makefile for setting
up your environment


## Certificate Generation Example

To run the test do:

```
./certgen_example
Loading CA certificate
Successfully read 666 bytes from ./ca-ecc-cert.der

Loading the CA key
Successfully read 121 bytes from ./ca-ecc-key.der
Decoding the CA private key
Successfully loaded CA Key

Generating a new ECC key
Successfully created new ECC key

Setting new cert issuer to subject of signer
Make Cert returned 490
Signed Cert returned 581
Successfully created new certificate

Writing newly generated DER certificate to file "./newCert.der"
Successfully output 581 bytes
Convert the DER cert to PEM formatted cert
Resulting PEM buffer is 843 bytes
Successfully converted the DER to PEM to "./newCert.pem"

Tests passed
```

You should see the following output when the cert is converted to human
readable format.

```
openssl x509 -inform pem -in newCert.pem -text

Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 81179639550048334 (0x1206873ba5ff84e)
    Signature Algorithm: ecdsa-with-SHA256
        Issuer: C=US, ST=Montana, L=Bozeman, O=Sawtooth, OU=Consulting, CN=www.wolfssl.com/emailAddress=info@wolfssl.com
        Validity
            Not Before: Jul 17 15:53:18 2017 GMT
            Not After : Nov 30 14:53:18 2018 GMT
        Subject: C=US, ST=MT, L=Bozeman, O=yourOrgNameHere, OU=yourUnitNameHere, CN=www.yourDomain.com/emailAddress=yourEmail@yourDomain.com
        Subject Public Key Info:
            Public Key Algorithm: id-ecPublicKey
                Public-Key: (256 bit)
                pub: 
                    04:15:62:0f:87:13:01:97:65:5c:62:a7:1c:92:bc:
                    61:df:24:52:ed:49:89:a1:ed:42:86:ad:dd:bf:1c:
                    a8:35:d3:9d:2c:29:12:cb:ce:05:bd:40:0b:24:f3:
                    d7:e0:61:f2:69:51:2a:20:b3:34:13:33:e7:69:b8:
                    d9:81:19:5f:b8
                ASN1 OID: prime256v1
                NIST CURVE: P-256
    Signature Algorithm: ecdsa-with-SHA256
         30:45:02:20:75:11:0c:e7:b3:73:20:88:d2:67:69:f2:1a:46:
         fb:d2:67:31:c7:c7:58:b4:9d:e2:48:95:db:bb:1f:1d:24:ab:
         02:21:00:d6:30:b9:c0:32:0d:42:74:56:b0:9e:8f:dc:83:1d:
         e6:a3:af:99:ea:03:97:4c:dc:d0:11:b8:10:a1:5a:29:a5
-----BEGIN CERTIFICATE-----
MIICNTCCAdugAwIBAgIIASBoc7pf+E4wCgYIKoZIzj0EAwIwgZQxCzAJBgNVBAYT
AlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdCb3plbWFuMREwDwYDVQQK
DAhTYXd0b290aDETMBEGA1UECwwKQ29uc3VsdGluZzEYMBYGA1UEAwwPd3d3Lndv
bGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29tMCIYDzIw
MTcwNzE3MTU1MzE4WhgPMjAxODExMzAxNDUzMThaMIGnMQswCQYDVQQGEwJVUzEL
MAkGA1UECAwCTVQxEDAOBgNVBAcMB0JvemVtYW4xGDAWBgNVBAoMD3lvdXJPcmdO
YW1lSGVyZTEZMBcGA1UECwwQeW91clVuaXROYW1lSGVyZTEbMBkGA1UEAwwSd3d3
LnlvdXJEb21haW4uY29tMScwJQYJKoZIhvcNAQkBFhh5b3VyRW1haWxAeW91ckRv
bWFpbi5jb20wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQVYg+HEwGXZVxipxyS
vGHfJFLtSYmh7UKGrd2/HKg1050sKRLLzgW9QAsk89fgYfJpUSogszQTM+dpuNmB
GV+4MAoGCCqGSM49BAMCA0gAMEUCIHURDOezcyCI0mdp8hpG+9JnMcfHWLSd4kiV
27sfHSSrAiEA1jC5wDINQnRWsJ6P3IMd5qOvmeoDl0zc0BG4EKFaKaU=
-----END CERTIFICATE-----

```


## Certificate Signing Request (CSR) Example

```
./csr_example ecc
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEICJ7jM8zdrZCoTdaeXfiNkRA0Wbf+JlATRLzMEghvGiToAoGCCqGSM49
AwEHoUQDQgAEdfzfuFaVgG1icB3Bwqkv27zZQdhUyOTHeN/4VbEoiB69EW5luFHy
6MWJEn+5a75Pp/dQKjlTb8Ukp/f7dRr8gg==
-----END EC PRIVATE KEY-----
 (227)
Saved Key PEM to "ecc-key.pem"
-----BEGIN CERTIFICATE REQUEST-----
MIIBTTCB8wIBAjCBkDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk9SMREwDwYDVQQH
DAhQb3J0bGFuZDEQMA4GA1UECgwHd29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1l
bnQxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5m
b0B3b2xmc3NsLmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABHX837hWlYBt
YnAdwcKpL9u82UHYVMjkx3jf+FWxKIgevRFuZbhR8ujFiRJ/uWu+T6f3UCo5U2/F
JKf3+3Ua/IKgADAKBggqhkjOPQQDAgNJADBGAiEAtaKt31WXJkFgI4dFw6dG45F3
Ia4BEV4KoPCBbMs81vICIQCj4IiuQpYH5dIsFuN8h0QGIfXTxdd9eqNsJJ1ElOXt
hA==
-----END CERTIFICATE REQUEST-----
 (530)
Saved CSR PEM to "ecc-csr.pem"
```

```
./csr_example ed25519
-----BEGIN EDDSA PRIVATE KEY-----
MFICAQAwBQYDK2VwBCIEIJAf0KRMwpoM8PcjTgNzMlJLtdGGml5kbZRJUlSChaxY
oSIEIBsUx1M7yeJiLIY6I/XrWX0VBcyp3UYa5r2IqLiA8Nrg
-----END EDDSA PRIVATE KEY-----
 (180)
Saved Key PEM to "ed25519-key.pem"
-----BEGIN CERTIFICATE REQUEST-----
MIIBETCBxAIBAjCBkDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk9SMREwDwYDVQQH
DAhQb3J0bGFuZDEQMA4GA1UECgwHd29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1l
bnQxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5m
b0B3b2xmc3NsLmNvbTAqMAUGAytlcAMhABsUx1M7yeJiLIY6I/XrWX0VBcyp3UYa
5r2IqLiA8NrgoAAwBQYDK2VwA0EAy3o01+L7OaB3qo825GQSKspWijGrFulU1BBQ
3z2Pr2lx6L87awbrWUtwvlXOGHQVl5ZjV+UkZURHMeNnS4Q2CQ==
-----END CERTIFICATE REQUEST-----
 (448)
Saved CSR PEM to "ed25519-csr.pem"
```


## CSR Signing with CA

This example shows how to use a CSR to sign it using a CA cert and key to produce an X.509 certificate.

```
% ./csr_sign ecc-csr.pem ca-ecc-cert.der ca-ecc-key.der
Loading CA certificate
Read 666 bytes from ca-ecc-cert.der

CA Cert file detected as DER

Loading the CA key
Read 121 bytes from ca-ecc-key.der
CA Key file detected as DER

Loading CA key to ecc_key struct
Loading CSR certificate
Successfully read 530 bytes from ecc-csr.pem

Converted CSR Cert PEM to DER 337 bytes
Loaded CSR to DecodedCert struct

Decoding Public Key
Setting certificate subject
Setting certificate issuer
Creating certificate...
Successfully created certificate 518

Signing certificate...
Successfully signed certificate 608

Writing newly generated DER certificate to file "./newCert.der"
Successfully output 608 bytes
Convert the DER cert to PEM formatted cert
Resulting PEM buffer is 879 bytes
Successfully converted the DER to PEM to "./newCert.pem"

Tests passed
```

## Certificate Generation Example with alt names

The alternate names feature is enabled with the wolfSSL build option 
`WOLFSSL_ALT_NAMES`. In the certgen_example.c see the `WOLFSSL_ALT_NAMES` 
sections for how to add this to a CSR.

Unfortunately wolfSSL does not yet have an API for this but this example shows
how to setup your own ASN.1 format string for using with the wolfSSL certificate
structure. TODO: Add an API for this!

Example of a cert being generated with this example

```
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            08:1a:be:1b:2e:5a:c5:aa:2c:e5:6d:db:20:22:31:b5
        Signature Algorithm: ecdsa-with-SHA256
        Issuer: C = US, ST = Montana, L = Bozeman, O = Sawtooth, OU = Consulting, CN = www.wolfssl.com, emailAddress = info@wolfssl.com
        Validity
            Not Before: May  6 21:14:47 2020 GMT
            Not After : Sep 19 21:14:47 2021 GMT
        Subject: C = US, ST = MT, L = Bozeman, O = yourOrgNameHere, OU = yourUnitNameHere, CN = www.yourDomain.com, emailAddress = yourEmail@yourDomain.com
        Subject Public Key Info:
            Public Key Algorithm: id-ecPublicKey
                Public-Key: (256 bit)
                pub:
                    04:8e:dc:b9:92:59:51:40:2e:3f:33:44:55:70:80:
                    16:bc:41:84:ab:47:3e:8b:93:6a:a0:16:78:0a:e9:
                    49:9a:d5:fe:08:cc:c3:23:2f:26:5a:14:cc:b1:8e:
                    db:94:8d:ad:3c:57:a4:3b:4f:e2:f0:7e:28:33:01:
                    40:57:f0:85:b5
                ASN1 OID: prime256v1
                NIST CURVE: P-256
        X509v3 extensions:
            X509v3 Subject Alternative Name: 
                DNS:localhost, DNS:example.com, DNS:127.0.0.1
    Signature Algorithm: ecdsa-with-SHA256
         30:44:02:20:36:08:d9:df:9e:7f:c2:1c:0c:db:06:26:3d:fe:
         8e:82:6e:64:07:6e:9b:fb:47:97:0a:d0:63:f6:6c:59:2a:82:
         02:20:37:5c:00:eb:0d:7d:95:51:5d:8e:e9:06:c7:a5:6f:7d:
         8b:1d:69:8d:8e:f8:5b:ba:13:0e:2a:5f:b4:86:1b:12
-----BEGIN CERTIFICATE-----
MIICbjCCAhWgAwIBAgIQCBq+Gy5axaos5W3bICIxtTAKBggqhkjOPQQDAjCBlDEL
MAkGA1UEBhMCVVMxEDAOBgNVBAgMB01vbnRhbmExEDAOBgNVBAcMB0JvemVtYW4x
ETAPBgNVBAoMCFNhd3Rvb3RoMRMwEQYDVQQLDApDb25zdWx0aW5nMRgwFgYDVQQD
DA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEWEGluZm9Ad29sZnNzbC5j
b20wIhgPMjAyMDA1MDYyMTE0NDdaGA8yMDIxMDkxOTIxMTQ0N1owgacxCzAJBgNV
BAYTAlVTMQswCQYDVQQIDAJNVDEQMA4GA1UEBwwHQm96ZW1hbjEYMBYGA1UECgwP
eW91ck9yZ05hbWVIZXJlMRkwFwYDVQQLDBB5b3VyVW5pdE5hbWVIZXJlMRswGQYD
VQQDDBJ3d3cueW91ckRvbWFpbi5jb20xJzAlBgkqhkiG9w0BCQEWGHlvdXJFbWFp
bEB5b3VyRG9tYWluLmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABI7cuZJZ
UUAuPzNEVXCAFrxBhKtHPouTaqAWeArpSZrV/gjMwyMvJloUzLGO25SNrTxXpDtP
4vB+KDMBQFfwhbWjMDAuMCwGA1UdEQQlMCOCCWxvY2FsaG9zdIILZXhhbXBsZS5j
b22CCTEyNy4wLjAuMTAKBggqhkjOPQQDAgNHADBEAiA2CNnfnn/CHAzbBiY9/o6C
bmQHbpv7R5cK0GP2bFkqggIgN1wA6w19lVFdjukGx6VvfYsdaY2O+Fu6Ew4qX7SG
GxI=
-----END CERTIFICATE-----
```


## Certificate Signing Request (CSR) Example with Crypto Callbacks

Example of generating a PEM-encoded certificate signing request (CSR) using the
Crypto Callbacks to show signing against HSM/TPM

Tested with these wolfSSL build options:

```sh
./autogen.sh  # If cloned from GitHub
./configure --enable-certreq --enable-certgen --enable-certext --enable-keygen --enable-cryptocb
make
make check
sudo make install
sudo ldconfig # required on some targets
```

### `csr_cryptocb` Example output

```
% ./csr_cryptocb
Invalid input supplied try one of the below examples
Examples:

./csr_cryptocb rsa
./csr_cryptocb ecc
./csr_cryptocb ed25519
```

```
% ./csr_cryptocb ecc
CryptoCb: PK ECDSA-Sign (4)
-----BEGIN CERTIFICATE REQUEST-----
MIIBTDCB8wIBAjCBkDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk9SMREwDwYDVQQH
DAhQb3J0bGFuZDEQMA4GA1UECgwHd29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1l
bnQxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5m
b0B3b2xmc3NsLmNvbTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABLszrEwnUErG
SqUEwzzenzbbci3OlOor+ssgCTksFuhhAumvTdMCk5oxW5eSIX/wzxjakRECNIbo
IFgzC4A0idigADAKBggqhkjOPQQDAgNIADBFAiEAmJK6ZapcFw4NTjImNoBrpudZ
6sFvRccABG35QyASvIsCIFXcN4Wo5qiH37OjT7dehbdZvQbJFw4hKOtim1gw5Z4p
-----END CERTIFICATE REQUEST-----
 (525)
Saved CSR PEM to "ecc-csr.pem"
```

```
% ./csr_cryptocb rsa
CryptoCb: PK RSA (1)
-----BEGIN CERTIFICATE REQUEST-----
MIIC1jCCAb4CAQIwgZAxCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJPUjERMA8GA1UE
BwwIUG9ydGxhbmQxEDAOBgNVBAoMB3dvbGZTU0wxFDASBgNVBAsMC0RldmVsb3Bt
ZW50MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0BCQEWEGlu
Zm9Ad29sZnNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDD
A9Er/jmkMkU7U8iEKyp8dJq9qipSB0fWpjayBzKO0Lppe8bDRJ7UgUj9LWiii2e7
oXXINixK0hv3i7rPDfnv7PGBHnubA0eav2XMf2UkaaboFIlb5DT3xbAUk/Vnezp6
eOEBVlaRphNCjdI8QJxM79GG3zdRGwyhO/Xxo0o15OHOlt8bfr9Ol9AQ6KgIMIGv
IAtDFMV0Z7Qygm+NhsKIQJk2g7oeQHIiF9dSZSRzsM7vGc2u/3hse8ASA9ROcg1Q
bTujO6OZXp3I2QyFs9mK2VQm2236rLv/JUzE0Xn0cdOGQBgTsGO1ck4wxJeEhi1W
L9cV93/ArvX8W+X7obrTAgMBAAGgADANBgkqhkiG9w0BAQsFAAOCAQEAK+RxSIQ4
CqxYbicHcbFZgHrTy7MOoA8uM5XWIP/AKZ4u64zbF2K6XIMItrfIGEouc6MDfKVh
W8ml6O75SR/7h/8S/9GClqcuzFqtNflzmb20Oadkl09sRIXPhFXMpt4V+Zc4cOka
kcqL0q+oEU+8naRJJ6pa6LL1NWRq79es4DykvtHcGkx0J1VPlu3ux7iYlQ0fVfRs
QUI1z3K52qfuchKW5zwkDjkVDbiWPd0/pbtUW2f8TAC21h0NabwMGISrrxmu6SX2
wI8YHCyLX5sMUosQ78vuGBD0bD5rxil0lzO9pjvWiSLZe0ubJ2Qq+FiuqIqITRiy
5AMQEs5siVCCEA==
-----END CERTIFICATE REQUEST-----
 (1062)
Saved CSR PEM to "rsa-csr.pem"
```

```
% ./csr_cryptocb ed25519
CryptoCb: PK ED25519-Sign (6)
-----BEGIN CERTIFICATE REQUEST-----
MIIBETCBxAIBAjCBkDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk9SMREwDwYDVQQH
DAhQb3J0bGFuZDEQMA4GA1UECgwHd29sZlNTTDEUMBIGA1UECwwLRGV2ZWxvcG1l
bnQxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5m
b0B3b2xmc3NsLmNvbTAqMAUGAytlcAMhAOZXWxMbx1EUa+079dH6q55stusCCaOZ
9W6/nTz+VDnmoAAwBQYDK2VwA0EAZX+pExcyCYxJHJmmISW8AxfhgJ/PmTPK4+uH
7iTt+VTNvi/Z6IPjlg+UWIbyyCno0RoNnB7GiqnvPn8fVAcsAw==
-----END CERTIFICATE REQUEST-----
 (448)
Saved CSR PEM to "ed25519-csr.pem"
```

## Certificate Generation and Parsing with Custom Extensions Example

Example of generating a PEM-encoded and DER-encoded certificate with custom
extensions. We then parse those certificates and display the custom extensions
using a callback that is called for each unknown extension that is encountered.

Tested with these wolfSSL build options:

```sh
./autogen.sh  # If cloned from GitHub
./configure --enable-asn=template --enable-certreq --enable-keygen --enable-certgen --enable-certext CFLAGS="-DWOLFSSL_TEST_CERT -DHAVE_OID_DECODING -DHAVE_OID_ENCODING -DWOLFSSL_CUSTOM_OID -DWOLFSSL_CERT_EXT"
make
make check
sudo make install
sudo ldconfig # required on some targets
```

In the directory where this README.md file is found, build and execute the
`custom_ext` and `custom_ext_callback` samples:

```sh
make custom_ext
make custom_ext_callback
./custom_ext
./custom_ext_callback newCert.der
```

For independent verification of the presence of the extensions, you can
pretty-print the certificates using `openssl`:

```
openssl x509 -in newCert.pem -noout -text
```

The output should be similar to this:
```
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            3c:26:f1:35:59:78:2c:1b:56:5f:3d:9c:be:eb:5a:1d
        Signature Algorithm: ecdsa-with-SHA256
        Issuer: C = US, ST = Washington, L = Seattle, O = wolfSSL, OU = Development, CN = www.wolfssl.com, emailAddress = info@wolfssl.com
        Validity
            Not Before: Mar  3 21:42:52 2022 GMT
            Not After : Jul 17 21:42:52 2023 GMT
        Subject: C = US, ST = MT, L = Bozeman, O = yourOrgNameHere, OU = yourUnitNameHere, CN = www.yourDomain.com, emailAddress = yourEmail@yourDomain.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:e2:3c:29:23:9f:3d:e4:07:09:2d:61:df:7b:5f:
                    ef:32:cd:17:84:b6:84:b7:44:90:39:39:77:6b:a3:
                    72:45:88:bd:3f:3a:8a:a7:1d:e6:f0:09:2c:ba:1a:
                    6b:cf:62:a8:a6:d5:5b:83:21:dc:0e:73:5f:0e:06:
                    f2:53:06:7c:c8:ea:67:82:df:79:4e:18:1b:e2:16:
                    cc:97:aa:d6:72:75:2f:1f:ca:65:e1:40:5b:95:e6:
                    d5:14:ea:de:f1:c1:39:c6:11:3d:a6:01:7b:63:57:
                    55:8e:b7:d4:54:2e:e2:83:18:a6:74:11:d1:38:87:
                    d0:83:09:80:22:0d:41:ac:cf:40:d4:a1:23:b9:97:
                    52:b1:e0:88:4d:48:b4:5e:c5:ef:63:c6:3c:e8:42:
                    d7:0d:b0:4a:fe:e1:c4:76:06:4a:a0:a9:0e:0c:45:
                    af:7f:ec:de:78:b8:53:7e:d1:a2:ea:9d:d6:12:3c:
                    a9:cb:88:2d:55:b6:fa:57:d0:28:3e:f1:c0:14:ce:
                    92:3a:6c:23:56:21:3f:e7:72:d5:8f:94:ee:be:fa:
                    86:d0:80:6b:3d:bd:ab:b3:5e:08:fb:50:c0:73:0c:
                    90:18:d3:c3:db:f9:62:56:7f:51:b2:c2:63:b1:00:
                    1c:6e:da:a3:06:07:52:57:d0:64:cd:a2:11:9f:2d:
                    93:6b
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            1.2.3.4.5: critical
                This is a critical extension
            1.2.3.4.6:
                This is NOT a critical extension
    Signature Algorithm: ecdsa-with-SHA256
         30:45:02:20:7b:5f:77:56:5d:c7:c7:06:8e:bf:c4:95:fa:cc:
         71:c8:e8:77:61:6c:7d:d3:84:1d:53:c3:2e:02:5c:77:06:e8:
         02:21:00:bf:f0:b4:0f:5c:fd:2b:16:92:35:43:7e:dc:59:bd:
         0f:8e:c0:82:e9:54:5d:57:7f:0e:e6:7d:5a:46:27:75:cb

```

Note the section titled "X509v3 extensions:".
