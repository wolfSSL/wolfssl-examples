# wolfSSL SSL/TLS Client Demo Application

This demo application runs a wolfSSL TLS example client. It connects to the
main wolfSSL example server (distributed with wolfSSL proper).

This example client uses SE050-based cryptography inside the module where
supported. It does not load the client certificate and private key into the
SE050 module. For a more advanced demo that does load these into the SE050
and uses the private key directly from the module based on key ID, see the
wolfSSL client demo located up a directory below, or read the associated
[README.md](../wolfssl_client_cert_key/README.md) for that demo.

```
wolfssl_client_cert_key/wolfssl_client_cert_key.c
```

## Building the Demo

Before building this demo, follow initial setup instructions in the parent
[README.md](../../README.md).

Compiling the middleware will also compile this demo application:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ cmake --build .
```

## Running the Demo

To run the demo, first start the wolfSSL example server. This demo client
assumes the server will be available at **127.0.0.1:11111**:

```
$ cd <wolfssl_directory>
$ ./examples/server/server
```

Then, to run this demo:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c/bin
$ ./wolfssl_client
```

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfssl_client
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
sss   :WARN :Communication channel is Plain.
sss   :WARN :!!!Not recommended for production use.!!!
App   :INFO :wolfSSL example client

App   :INFO :Running wc_se050_set_config()
App   :INFO :SE050 config successfully set in wolfSSL
App   :INFO :Created and configured socket
App   :INFO :Socket connected
App   :INFO :Created WOLFSSL_CTX
App   :INFO :Created WOLFSSL_CTX
App   :INFO :Loaded CA certs into CTX
App   :INFO :Loaded client certificate into CTX
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
sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id 10006
App   :INFO :Sending message to server: GET /index.html HTTP/1.0



App   :INFO :wolfSSL_write sent 28 bytes

App   :INFO :Server response: I hear you fa shizzle!

App   :INFO :TLS shutdown not complete
App   :INFO :TLS shutdown complete
App   :INFO :Done with sample app
App   :INFO :ex_sss Finished
```

## Demo Notes

It is expected to see warning messages in the console log about failures to
delete key ids. wolfCrypt internally tries to delete temporary/test keys,
and if those keys have not been stored in the SE050 the debug logs will show
these messages.

See the source code of the demo for more notes about wolfSSL API usage.

