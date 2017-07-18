To build this example configure wolfssl with --enable-certgen

If having issues building please check comments in the Makefile for setting
up your environment

To run the test do:

```
make
./run_certgen_example
```

You should see the following output when the cert is converted to human
readable format.

```
ertificate:
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

