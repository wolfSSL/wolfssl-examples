# wolfSSL SSL/TLS Client Demo Application with Cert and Private Key in SE050

This demo application runs a wolfSSL TLS example client. It connects to the
main wolfSSL example server (distributed with wolfSSL proper).

This example client uses SE050-based cryptography inside the module where
supported. It also loads a client certificate and private RSA or ECC key into
the SE050 module. The certificate is extracted back out to be loaded into
wolfSSL context when needed. The private key is left in the SE050 module and
associated private key operations are done inside the SE050 using the matching
SE050 key ID.

If looking for a more simple example that does not load the client certificate
and private key into the SE050 module, see the wolfSSL client demo located up a
directory below, or read the associated
[README.md](../wolfssl_client/README.md) for that demo.

```
wolfssl_client/wolfssl_client.c
```

## Building the Demo

Before building this demo, follow initial setup instructions in the parent
[README.md](../../README.md).

The wolfSSL library linked against will need to have `-DWOLF_PRIVATE_KEY_ID`
defined when configuring/building the library. This enables the use of a
key ID with the SE050 through the wolfSSL SSL/TLS API. This should be added
to CFLAGS when doing ./configure (or preprocessor defines if not using
configure). For example, wolfSSL configuration used when testing this demo
on Raspberry Pi with an SE050 EdgeLock dev kit:

```
$ cd <wolfssl_directory>
$ ./configure --with-se050 --enable-certgen --disable-examples CFLAGS="-DSIZEOF_LONG_LONG=8 -DSE050_KEYID_START=0x0000FFFF -DWOLFSSL_SE050_NO_TRNG -DWOLF_PRIVATE_KEY_ID"
```

Compiling the middleware will also compile this demo application:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ cmake --build .
```

## Running the Demo

To run the demo, first start the wolfSSL example server. This demo client
assumes the server will be available at **127.0.0.1:11111**. The example
server will need to be started differently depending on how the SE050 demo
client has been configured (see source code for optional defines). The default
configuration uses RSA certs and keys, which require the wolfSSL example
server to be started with default credentials:

```
$ cd <wolfssl_directory>
$ ./examples/server/server
```

If the SE050 client demo has been configured to use ECC certs and keys,
by defining `EXAMPLE_USE_RSA = 0`, then the wolfSSL example server should
be started using:

```
$ cd <wolfssl_directory>
$ ./examples/server/server -c ./certs/server-ecc.pem -k ./certs/ecc-key.pem \
    -A ./certs/client-ecc-cert.pem 
```

Then, to run this demo:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c/bin
$ ./wolfssl_client_cert_key
```

The demo can be configured to load and use the client certificate from the
SE050 after loading in with the `EXAMPLE_READ_CERT_FROM_SE050` define.

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfssl_client_cert_key
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
sss   :WARN :Communication channel is Plain.
sss   :WARN :!!!Not recommended for production use.!!!
App   :INFO :------------------------------------------------------------------
App   :INFO :wolfSSL example client
App   :INFO :Using RSA certs/keys inside SE050
App   :INFO :------------------------------------------------------------------
App   :INFO :Running wc_se050_set_config()
App   :INFO :SE050 config successfully set in wolfSSL
App   :INFO :Inserted RSA private key into SE050 key ID: 0x00005002

App   :INFO :Inserted RSA certificate into SE050 key ID: 0x00005003

App   :INFO :Created and configured socket
App   :INFO :Socket connected
App   :INFO :Created WOLFSSL_CTX
App   :INFO :Loaded client private key into CTX
App   :INFO :Created new WOLFSSL
App   :INFO :Set wolfSSL fd
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id FFFF
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10000
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10001
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10002
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10003
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10004
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10005
App   :INFO :Sending message to server: GET /index.html HTTP/1.0



App   :INFO :wolfSSL_write sent 28 bytes
App   :INFO :Server response: I hear you fa shizzle!

App   :INFO :TLS shutdown not complete
App   :INFO :TLS shutdown complete
App   :INFO :Erased RSA private key, key ID: 0x00005002
App   :INFO :Erased RSA certificate, key ID: 0x00005003
App   :INFO :Done with sample app
App   :INFO :ex_sss Finished
```

## Demo Notes

It is expected to see warning messages in the console log about failures to
delete key ids. wolfCrypt internally tries to delete temporary/test keys,
and if those keys have not been stored in the SE050 the debug logs will show
these messages.

See the source code of the demo for more notes about wolfSSL API usage.

