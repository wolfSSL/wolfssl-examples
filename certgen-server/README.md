This test is to show an example of creating a signed server certificate at run time and having a web browser consider it to be secure.

To build this example configure wolfssl with --enable-certgen --enable-keygen C_EXTRA_FLAGS=-DUSE_CERT_BUFFERS_2048 and optionally --enable-certext C_EXTRA_FLAGS=-DWOLFSSL_ALT_NAMES

```
./configure --enable-certgen --enable-keygen --enable-certext C_EXTRA_FLAGS="-DUSE_CERT_BUFFERS_2048 -DWOLFSSL_ALT_NAMES"
```


To run the test:

If using a web browser then load the certificate wolfssl-root/certs/ca-cert.pem as a trusted certificate in the web browser. This certificate can verify server-cert.pem which has been used to create the new certificate.

The certificate chain will be ca-cert.pem -> server-cert.pem -> created-cert.pem. The chain sent by the server is server-cert.pem -> created-cert.pem. To create the example server and run it use the following commands from the directory "certgen-server/".

```
make
./server
```

After the server is started three files get output. created_chain.pem, created_cert.der, and created_key.der. Then it sits in a ready state waiting for a connection. To connect to the server open up the web server that has had ca-cert.pem loaded and type the URL "https://127.0.0.1:11111". Or connect using wolfSSL's example client using server-cert.pem as the CA authority.

```
cd wolfssl-root
./examples/client/client -A ./certs/server-cert.pem
```

NOTE:
Google Chrome requires a SAN extension. This is supported with the listed optional wolfSSL build settings and is loaded in from san.pem.
