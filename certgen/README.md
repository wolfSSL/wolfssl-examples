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
% ./csr_sign ecc ecc-csr.pem ca-ecc-cert.der ca-ecc-key.der
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

Pretty much the same as the certgen_example but adds some alt names to the cert
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
