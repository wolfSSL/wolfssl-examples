# wolfSSL CertManager Example

This directory contains:

A simple example of using the wolfSSL CertManager to verify a certificate
in a standalone manner, separate from an SSL/TLS connection.

## Compiling and Running the Example

```
$ make
$ ./certverify
```
## Verification of OQS Falcon Certificates

The `generate_falcon_chains.sh` script will allow you to use the OQS project's
OpenSSL in order to generate a self-signed CA certificate and entity
certificate that use Falcon. In the OpenSSL directory, run the script to
generate the certificates in the the /tmp/ directory.

Apply the following patch:

```
diff --git a/certmanager/certverify.c b/certmanager/certverify.c
index 4b5fed7..1b29d89 100644
--- a/certmanager/certverify.c
+++ b/certmanager/certverify.c
@@ -25,13 +25,15 @@
 #include <wolfssl/wolfcrypt/error-crypt.h>
 #include <wolfssl/test.h>
 
+#undef HAVE_CRL
+
 int main(void)
 {
     int ret;
     WOLFSSL_CERT_MANAGER* cm = NULL;
 
-    const char* caCert     = "../certs/ca-cert.pem";
-    const char* verifyCert = "../certs/server-cert.pem";
+    const char* caCert     = "/tmp/falcon1024_root_cert.pem";
+    const char* verifyCert = "/tmp/falcon1024_entity_cert.pem";
 
 #ifdef HAVE_CRL
     const char* crlPem     = "../certs/crl/crl.pem";
@@ -52,7 +54,7 @@ int main(void)
         return -1;
     }
 
-    wolfSSL_CertManagerSetVerify(cm, myVerify);
+    //wolfSSL_CertManagerSetVerify(cm, myVerify);
 
     ret = wolfSSL_CertManagerLoadCA(cm, caCert, NULL);
     if (ret != SSL_SUCCESS) {
```

Then compile and run the sample:
```
$ make
$ ./certverify
```

