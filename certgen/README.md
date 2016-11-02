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
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number:
            01:68:de:48:eb:aa:76:e6
        Signature Algorithm: ecdsa-with-SHA256
        Issuer: C=US, ST=Montana, L=Bozeman, O=Sawtooth, OU=Consulting, CN=www.wolfssl.com/emailAddress=info@wolfssl.com
        Validity
            Not Before: Nov  2 00:07:29 2016 GMT
            Not After : Mar 18 00:07:29 2018 GMT
        Subject: 
        Subject Public Key Info:
            Public Key Algorithm: id-ecPublicKey
            EC Public Key:
                pub: 
                    04:f9:4b:dd:9e:b9:bc:5b:e4:ac:a3:a5:6f:60:96:
                    3e:9d:d2:06:50:f8:34:5d:5b:c6:ce:52:7e:1d:ab:
                    6b:51:06:6f:e5:c2:da:b6:09:9f:20:9f:82:01:90:
                    ca:33:13:22:38:23:9e:84:b3:b6:23:16:7a:8b:d2:
                    13:97:9d:7a:de
                ASN1 OID: prime256v1
    Signature Algorithm: ecdsa-with-SHA256
        30:45:02:21:00:dd:8c:97:1f:e8:56:dc:f1:2a:fd:f6:86:bf:
        a7:c6:11:fc:9b:3f:09:8c:c2:ad:0e:0a:f9:2c:97:40:c6:ff:
        dc:02:20:61:0b:1b:d7:be:69:3a:dd:fe:77:ca:0d:74:b0:ba:
        a2:0c:1a:7a:bf:2e:c5:e7:46:11:8b:04:9e:27:ba:7c:27
```

