#pragma once

/* 
embedded version of CA_FILE

example to generate this CA_FILE text for a new file:

sed 's/\(.*\)\r/"\1\\n"/g' ca-cert.pem

see also this DER to C script:

https://github.com/wolfSSL/wolfssl/blob/master/scripts/dertoc.pl

*/


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
const unsigned char * CA_FILE = ""
    
/* TODO this is a test example! DO NOT USE IN PRODUCTION!! */

/* insert your ca-cert.pem text below */ 
"Certificate:\n"
"    Data:\n"
"        Version: 3 (0x2)\n"
"        Serial Number:\n"
"            5e:ba:a4:f4:b1:f7:48:25:e3:5f:9b:da:a1:13:ed:d5:2b:03:67:15\n"
"        Signature Algorithm: sha256WithRSAEncryption\n"
"        Issuer: C = US, ST = Montana, L = Bozeman, O = Sawtooth, OU = Consulting, CN = www.wolfssl.com, emailAddress = info@wolfssl.com\n"
"        Validity\n"
"            Not Before: Jun 19 13:23:41 2020 GMT\n"
"            Not After : Mar 16 13:23:41 2023 GMT\n"
"        Subject: C = US, ST = Montana, L = Bozeman, O = Sawtooth, OU = Consulting, CN = www.wolfssl.com, emailAddress = info@wolfssl.com\n"
"        Subject Public Key Info:\n"
"            Public Key Algorithm: rsaEncryption\n"
"                RSA Public-Key: (2048 bit)\n"
"                Modulus:\n"
"                    00:bf:0c:ca:2d:14:b2:1e:84:42:5b:cd:38:1f:4a:\n"
"                    f2:4d:75:10:f1:b6:35:9f:df:ca:7d:03:98:d3:ac:\n"
"                    de:03:66:ee:2a:f1:d8:b0:7d:6e:07:54:0b:10:98:\n"
"                    21:4d:80:cb:12:20:e7:cc:4f:de:45:7d:c9:72:77:\n"
"                    32:ea:ca:90:bb:69:52:10:03:2f:a8:f3:95:c5:f1:\n"
"                    8b:62:56:1b:ef:67:6f:a4:10:41:95:ad:0a:9b:e3:\n"
"                    a5:c0:b0:d2:70:76:50:30:5b:a8:e8:08:2c:7c:ed:\n"
"                    a7:a2:7a:8d:38:29:1c:ac:c7:ed:f2:7c:95:b0:95:\n"
"                    82:7d:49:5c:38:cd:77:25:ef:bd:80:75:53:94:3c:\n"
"                    3d:ca:63:5b:9f:15:b5:d3:1d:13:2f:19:d1:3c:db:\n"
"                    76:3a:cc:b8:7d:c9:e5:c2:d7:da:40:6f:d8:21:dc:\n"
"                    73:1b:42:2d:53:9c:fe:1a:fc:7d:ab:7a:36:3f:98:\n"
"                    de:84:7c:05:67:ce:6a:14:38:87:a9:f1:8c:b5:68:\n"
"                    cb:68:7f:71:20:2b:f5:a0:63:f5:56:2f:a3:26:d2:\n"
"                    b7:6f:b1:5a:17:d7:38:99:08:fe:93:58:6f:fe:c3:\n"
"                    13:49:08:16:0b:a7:4d:67:00:52:31:67:23:4e:98:\n"
"                    ed:51:45:1d:b9:04:d9:0b:ec:d8:28:b3:4b:bd:ed:\n"
"                    36:79\n"
"                Exponent: 65537 (0x10001)\n"
"        X509v3 extensions:\n"
"            X509v3 Subject Key Identifier: \n"
"                27:8E:67:11:74:C3:26:1D:3F:ED:33:63:B3:A4:D8:1D:30:E5:E8:D5\n"
"            X509v3 Authority Key Identifier: \n"
"                keyid:27:8E:67:11:74:C3:26:1D:3F:ED:33:63:B3:A4:D8:1D:30:E5:E8:D5\n"
"                DirName:/C=US/ST=Montana/L=Bozeman/O=Sawtooth/OU=Consulting/CN=www.wolfssl.com/emailAddress=info@wolfssl.com\n"
"                serial:5E:BA:A4:F4:B1:F7:48:25:E3:5F:9B:DA:A1:13:ED:D5:2B:03:67:15\n"
"\n"
"            X509v3 Basic Constraints: \n"
"                CA:TRUE\n"
"            X509v3 Subject Alternative Name: \n"
"                DNS:example.com, IP Address:127.0.0.1\n"
"            X509v3 Extended Key Usage: \n"
"                TLS Web Server Authentication, TLS Web Client Authentication\n"
"    Signature Algorithm: sha256WithRSAEncryption\n"
"         b9:ed:94:3e:00:73:2d:a5:d1:04:b3:fb:dc:f0:b7:0d:3d:ad:\n"
"         96:74:4c:92:67:ad:6d:7c:e2:99:6a:33:ca:b2:0f:04:5a:a5:\n"
"         67:f8:e3:0b:3d:f5:d0:5b:1e:20:52:12:92:28:ea:31:a3:51:\n"
"         9e:8b:d2:39:e4:25:ea:61:61:41:16:2d:54:50:d3:fb:d0:34:\n"
"         00:10:f1:7b:bc:f0:08:a7:f5:27:5e:7e:40:9d:99:b0:d3:31:\n"
"         11:c3:9d:a9:51:a0:17:cf:83:2c:55:84:e0:d5:92:a0:05:3a:\n"
"         9f:b8:75:f8:1b:e5:f7:a4:6c:e9:aa:25:8b:19:93:46:1f:3f:\n"
"         33:af:47:29:cf:7b:8b:59:27:eb:d7:4f:cb:33:19:fa:5f:ee:\n"
"         d8:13:e9:0c:07:ad:3b:c0:7f:10:d7:e4:ed:e8:db:16:e1:1f:\n"
"         a4:7f:16:3c:bd:d7:11:f2:d4:3a:a9:9b:95:e1:39:51:99:eb:\n"
"         5b:65:46:ef:63:84:73:95:23:b8:bf:b5:f6:4d:12:71:f7:ff:\n"
"         33:aa:4a:8c:65:73:73:89:69:df:a6:dc:a4:91:ff:ae:c7:28:\n"
"         93:b5:1a:de:a9:8f:2b:30:85:83:8b:99:82:ca:b3:7c:11:10:\n"
"         88:9d:8e:6c:2c:f3:05:6f:cb:80:85:16:b7:ed:e4:68:fb:b6:\n"
"         b6:31:8a:7d\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIE/zCCA+egAwIBAgIUXrqk9LH3SCXjX5vaoRPt1SsDZxUwDQYJKoZIhvcNAQEL\n"
"BQAwgZQxCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdC\n"
"b3plbWFuMREwDwYDVQQKDAhTYXd0b290aDETMBEGA1UECwwKQ29uc3VsdGluZzEY\n"
"MBYGA1UEAwwPd3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdv\n"
"bGZzc2wuY29tMB4XDTIwMDYxOTEzMjM0MVoXDTIzMDMxNjEzMjM0MVowgZQxCzAJ\n"
"BgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdCb3plbWFuMREw\n"
"DwYDVQQKDAhTYXd0b290aDETMBEGA1UECwwKQ29uc3VsdGluZzEYMBYGA1UEAwwP\n"
"d3d3LndvbGZzc2wuY29tMR8wHQYJKoZIhvcNAQkBFhBpbmZvQHdvbGZzc2wuY29t\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvwzKLRSyHoRCW804H0ry\n"
"TXUQ8bY1n9/KfQOY06zeA2buKvHYsH1uB1QLEJghTYDLEiDnzE/eRX3Jcncy6sqQ\n"
"u2lSEAMvqPOVxfGLYlYb72dvpBBBla0Km+OlwLDScHZQMFuo6AgsfO2nonqNOCkc\n"
"rMft8nyVsJWCfUlcOM13Je+9gHVTlDw9ymNbnxW10x0TLxnRPNt2Osy4fcnlwtfa\n"
"QG/YIdxzG0ItU5z+Gvx9q3o2P5jehHwFZ85qFDiHqfGMtWjLaH9xICv1oGP1Vi+j\n"
"JtK3b7FaF9c4mQj+k1hv/sMTSQgWC6dNZwBSMWcjTpjtUUUduQTZC+zYKLNLve02\n"
"eQIDAQABo4IBRTCCAUEwHQYDVR0OBBYEFCeOZxF0wyYdP+0zY7Ok2B0w5ejVMIHU\n"
"BgNVHSMEgcwwgcmAFCeOZxF0wyYdP+0zY7Ok2B0w5ejVoYGapIGXMIGUMQswCQYD\n"
"VQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96ZW1hbjERMA8G\n"
"A1UECgwIU2F3dG9vdGgxEzARBgNVBAsMCkNvbnN1bHRpbmcxGDAWBgNVBAMMD3d3\n"
"dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbYIU\n"
"Xrqk9LH3SCXjX5vaoRPt1SsDZxUwDAYDVR0TBAUwAwEB/zAcBgNVHREEFTATggtl\n"
"eGFtcGxlLmNvbYcEfwAAATAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n"
"DQYJKoZIhvcNAQELBQADggEBALntlD4Acy2l0QSz+9zwtw09rZZ0TJJnrW184plq\n"
"M8qyDwRapWf44ws99dBbHiBSEpIo6jGjUZ6L0jnkJephYUEWLVRQ0/vQNAAQ8Xu8\n"
"8Ain9SdefkCdmbDTMRHDnalRoBfPgyxVhODVkqAFOp+4dfgb5fekbOmqJYsZk0Yf\n"
"PzOvRynPe4tZJ+vXT8szGfpf7tgT6QwHrTvAfxDX5O3o2xbhH6R/Fjy91xHy1Dqp\n"
"m5XhOVGZ61tlRu9jhHOVI7i/tfZNEnH3/zOqSoxlc3OJad+m3KSR/67HKJO1Gt6p\n"
"jyswhYOLmYLKs3wREIidjmws8wVvy4CFFrft5Gj7trYxin0=\n"
"-----END CERTIFICATE-----\n"
/* insert your ca-cert.pem text above */    
"\x00"
; 

/* an long integer calculation of the size of our CA_FILE */
long sizeof_CA_FILE() {
    unsigned char * p;
    p = strchr(CA_FILE, 0);
    return p - CA_FILE + 1;
}

#pragma GCC diagnostic pop

    