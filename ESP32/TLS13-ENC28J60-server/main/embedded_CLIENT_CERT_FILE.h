#pragma once

/* 
embedded version of CA_FILE

#define CA_FILE   "../../../../certs/client-cert.pem"

example to generate this CA_FILE text for a new file:

sed 's/\(.*\)\r/"\1\\n"/g' ../../../certs/client-cert.pem

see also this DER to C script:

https://github.com/wolfSSL/wolfssl/blob/master/scripts/dertoc.pl

*/


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
const unsigned char * CA_FILE = ""
    
/* TODO this is a test example! DO NOT USE IN PRODUCTION!! */

/* insert your client-cert.pem text below */ 
"Certificate:\n"
"    Data:\n"
"        Version: 3 (0x2)\n"
"        Serial Number:\n"
"            45:45:81:82:e6:3a:bd:a2:e3:06:0e:ba:2c:45:74:4b:be:c0:39:11\n"
"        Signature Algorithm: sha256WithRSAEncryption\n"
"        Issuer: C = US, ST = Montana, L = Bozeman, O = wolfSSL_2048, OU = Programming-2048, CN = www.wolfssl.com, emailAddress = info@wolfssl.com\n"
"        Validity\n"
"            Not Before: Jun 19 13:23:41 2020 GMT\n"
"            Not After : Mar 16 13:23:41 2023 GMT\n"
"        Subject: C = US, ST = Montana, L = Bozeman, O = wolfSSL_2048, OU = Programming-2048, CN = www.wolfssl.com, emailAddress = info@wolfssl.com\n"
"        Subject Public Key Info:\n"
"            Public Key Algorithm: rsaEncryption\n"
"                RSA Public-Key: (2048 bit)\n"
"                Modulus:\n"
"                    00:c3:03:d1:2b:fe:39:a4:32:45:3b:53:c8:84:2b:\n"
"                    2a:7c:74:9a:bd:aa:2a:52:07:47:d6:a6:36:b2:07:\n"
"                    32:8e:d0:ba:69:7b:c6:c3:44:9e:d4:81:48:fd:2d:\n"
"                    68:a2:8b:67:bb:a1:75:c8:36:2c:4a:d2:1b:f7:8b:\n"
"                    ba:cf:0d:f9:ef:ec:f1:81:1e:7b:9b:03:47:9a:bf:\n"
"                    65:cc:7f:65:24:69:a6:e8:14:89:5b:e4:34:f7:c5:\n"
"                    b0:14:93:f5:67:7b:3a:7a:78:e1:01:56:56:91:a6:\n"
"                    13:42:8d:d2:3c:40:9c:4c:ef:d1:86:df:37:51:1b:\n"
"                    0c:a1:3b:f5:f1:a3:4a:35:e4:e1:ce:96:df:1b:7e:\n"
"                    bf:4e:97:d0:10:e8:a8:08:30:81:af:20:0b:43:14:\n"
"                    c5:74:67:b4:32:82:6f:8d:86:c2:88:40:99:36:83:\n"
"                    ba:1e:40:72:22:17:d7:52:65:24:73:b0:ce:ef:19:\n"
"                    cd:ae:ff:78:6c:7b:c0:12:03:d4:4e:72:0d:50:6d:\n"
"                    3b:a3:3b:a3:99:5e:9d:c8:d9:0c:85:b3:d9:8a:d9:\n"
"                    54:26:db:6d:fa:ac:bb:ff:25:4c:c4:d1:79:f4:71:\n"
"                    d3:86:40:18:13:b0:63:b5:72:4e:30:c4:97:84:86:\n"
"                    2d:56:2f:d7:15:f7:7f:c0:ae:f5:fc:5b:e5:fb:a1:\n"
"                    ba:d3\n"
"                Exponent: 65537 (0x10001)\n"
"        X509v3 extensions:\n"
"            X509v3 Subject Key Identifier: \n"
"                33:D8:45:66:D7:68:87:18:7E:54:0D:70:27:91:C7:26:D7:85:65:C0\n"
"            X509v3 Authority Key Identifier: \n"
"                keyid:33:D8:45:66:D7:68:87:18:7E:54:0D:70:27:91:C7:26:D7:85:65:C0\n"
"                DirName:/C=US/ST=Montana/L=Bozeman/O=wolfSSL_2048/OU=Programming-2048/CN=www.wolfssl.com/emailAddress=info@wolfssl.com\n"
"                serial:45:45:81:82:E6:3A:BD:A2:E3:06:0E:BA:2C:45:74:4B:BE:C0:39:11\n"
"\n"
"            X509v3 Basic Constraints: \n"
"                CA:TRUE\n"
"            X509v3 Subject Alternative Name: \n"
"                DNS:example.com, IP Address:127.0.0.1\n"
"            X509v3 Extended Key Usage: \n"
"                TLS Web Server Authentication, TLS Web Client Authentication\n"
"    Signature Algorithm: sha256WithRSAEncryption\n"
"         c1:06:39:c8:ce:f5:81:49:55:e1:3a:55:aa:91:5d:64:f1:4b:\n"
"         dc:33:1e:31:15:e7:10:71:16:0d:b5:00:dc:bb:22:0d:81:d9:\n"
"         12:5c:0c:dd:61:e1:af:b5:e2:f7:7d:8b:e6:61:fd:ca:45:3a:\n"
"         61:e7:18:56:2c:26:f2:df:14:f1:e0:3d:7f:62:12:5f:d8:04:\n"
"         44:06:0c:72:b1:8a:50:72:67:77:74:01:ff:79:64:11:6e:b3:\n"
"         84:51:19:22:b6:43:10:06:0d:39:46:5a:c6:57:0a:43:a1:94:\n"
"         02:28:0a:12:38:85:04:0e:78:52:48:28:7e:6c:d9:f0:4b:04:\n"
"         55:7c:39:01:c1:b4:5f:50:06:cf:da:6f:20:b8:94:f7:51:1a:\n"
"         23:cb:30:e3:21:b4:a3:73:ad:48:fb:96:69:ef:2e:50:b6:67:\n"
"         bc:64:ee:27:76:43:7a:34:59:8e:b4:57:53:7d:95:7e:50:7e:\n"
"         64:4c:29:68:fe:81:4f:73:21:24:b5:a9:a2:49:5f:54:7f:0d:\n"
"         c2:96:c7:f5:36:81:8e:c0:00:af:b4:10:6b:0e:bc:1a:3a:f1:\n"
"         a6:dd:fd:8e:63:21:33:d4:32:31:1f:be:a3:7c:52:85:26:c8:\n"
"         6e:50:6c:ac:3d:2e:c0:f9:50:ed:c8:4d:cd:13:50:ce:5e:de:\n"
"         15:89:d1:fb\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIFHTCCBAWgAwIBAgIURUWBguY6vaLjBg66LEV0S77AOREwDQYJKoZIhvcNAQEL\n"
"BQAwgZ4xCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAwDgYDVQQHDAdC\n"
"b3plbWFuMRUwEwYDVQQKDAx3b2xmU1NMXzIwNDgxGTAXBgNVBAsMEFByb2dyYW1t\n"
"aW5nLTIwNDgxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJ\n"
"ARYQaW5mb0B3b2xmc3NsLmNvbTAeFw0yMDA2MTkxMzIzNDFaFw0yMzAzMTYxMzIz\n"
"NDFaMIGeMQswCQYDVQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwH\n"
"Qm96ZW1hbjEVMBMGA1UECgwMd29sZlNTTF8yMDQ4MRkwFwYDVQQLDBBQcm9ncmFt\n"
"bWluZy0yMDQ4MRgwFgYDVQQDDA93d3cud29sZnNzbC5jb20xHzAdBgkqhkiG9w0B\n"
"CQEWEGluZm9Ad29sZnNzbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQDDA9Er/jmkMkU7U8iEKyp8dJq9qipSB0fWpjayBzKO0Lppe8bDRJ7UgUj9\n"
"LWiii2e7oXXINixK0hv3i7rPDfnv7PGBHnubA0eav2XMf2UkaaboFIlb5DT3xbAU\n"
"k/Vnezp6eOEBVlaRphNCjdI8QJxM79GG3zdRGwyhO/Xxo0o15OHOlt8bfr9Ol9AQ\n"
"6KgIMIGvIAtDFMV0Z7Qygm+NhsKIQJk2g7oeQHIiF9dSZSRzsM7vGc2u/3hse8AS\n"
"A9ROcg1QbTujO6OZXp3I2QyFs9mK2VQm2236rLv/JUzE0Xn0cdOGQBgTsGO1ck4w\n"
"xJeEhi1WL9cV93/ArvX8W+X7obrTAgMBAAGjggFPMIIBSzAdBgNVHQ4EFgQUM9hF\n"
"Ztdohxh+VA1wJ5HHJteFZcAwgd4GA1UdIwSB1jCB04AUM9hFZtdohxh+VA1wJ5HH\n"
"JteFZcChgaSkgaEwgZ4xCzAJBgNVBAYTAlVTMRAwDgYDVQQIDAdNb250YW5hMRAw\n"
"DgYDVQQHDAdCb3plbWFuMRUwEwYDVQQKDAx3b2xmU1NMXzIwNDgxGTAXBgNVBAsM\n"
"EFByb2dyYW1taW5nLTIwNDgxGDAWBgNVBAMMD3d3dy53b2xmc3NsLmNvbTEfMB0G\n"
"CSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbYIURUWBguY6vaLjBg66LEV0S77A\n"
"OREwDAYDVR0TBAUwAwEB/zAcBgNVHREEFTATggtleGFtcGxlLmNvbYcEfwAAATAd\n"
"BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDQYJKoZIhvcNAQELBQADggEB\n"
"AMEGOcjO9YFJVeE6VaqRXWTxS9wzHjEV5xBxFg21ANy7Ig2B2RJcDN1h4a+14vd9\n"
"i+Zh/cpFOmHnGFYsJvLfFPHgPX9iEl/YBEQGDHKxilByZ3d0Af95ZBFus4RRGSK2\n"
"QxAGDTlGWsZXCkOhlAIoChI4hQQOeFJIKH5s2fBLBFV8OQHBtF9QBs/abyC4lPdR\n"
"GiPLMOMhtKNzrUj7lmnvLlC2Z7xk7id2Q3o0WY60V1N9lX5QfmRMKWj+gU9zISS1\n"
"qaJJX1R/DcKWx/U2gY7AAK+0EGsOvBo68abd/Y5jITPUMjEfvqN8UoUmyG5QbKw9\n"
"LsD5UO3ITc0TUM5e3hWJ0fs=\n"
"-----END CERTIFICATE-----\n"
/* insert your client-cert.pem text above */    
"\x00"
; 

/* an long integer calculation of the size of our CA_FILE */
long sizeof_CA_FILE() {
    unsigned char * p;
    p = strchr(CA_FILE, 0);
    return p - CA_FILE + 1;
}

#pragma GCC diagnostic pop