# OCSP Response Parsing Example

This example shows how to manually parse an OCSP response.
 
Note: This requires access to local wolfSSL functions, which is why a static library is used.

```
./configure --enable-ocsp --enable-debug --disable-shared CFLAGS="-DWOLFSSL_NO_OCSP_DATE_CHECK -DWOLFSSL_NO_ASN_STRICT"
make
gcc -g -I. -o ocsp_response src/.libs/libwolfssl.a ocsp_response.c
./ocsp_response ca.der cert.der ocsp_resp.bin
CompareOcspReqResp Result: 0 (success)
```
