# wolfCrypt SE050 Key and Certificate Insertion and Use Example

This example inserts and uses keys and certificates into SE050, including:

1. Inserts sample RSA and ECC certs and keys into SE050 key IDs. Sample keys
and certs used are from wolfSSL's `<wolfssl/certs_test.h>` header and are demo
certs/keys that ship with wolfSSL:

    ```
    server_key_der_2048    RSA private key (2048-bit)
    server_cert_der_2048   RSA cert (2048-bit), matches above key
    public key from above  RSA public key extracted from above cert
    ecc_key_der_256        ECC private key (NIST P-256)
    serv_ecc_der_256       ECC cert (NIST P-256), matches above key
    public key from above  ECC public key extracted from above cert
    ```

2. Does an ECDSA verify using one of the ECC public keys and a ECDSA signature
in the form of ASCII R and S components.

3. Reads back out the RSA and ECC certificate inserted, verifies they match the
original inserted.

4. Erases the keys and certs from the SE050.

## Building the Demo

Before building this demo, follow initial setup instructions in the parent
[README.md](../../README.md).

Once this example directory has been copied into the SE05x middleware directory
tree in the correct location, compiling the middleware will also compile this
demo application:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c
$ cmake --build .
```

## Running the Demo

To run the demo:

```
$ cd /home/pi/se_mw/simw-top_build/raspbian_native_se050_t1oi2c/bin
$ ./wolfcrypt_key_cert_insert
```

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfcrypt_key_cert_insert
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
sss   :WARN :Communication channel is Plain.
sss   :WARN :!!!Not recommended for production use.!!!
App   :INFO :Running wc_se050_set_config()
App   :INFO :SE050 config successfully set in wolfSSL
App   :INFO :Inserted RSA private key into SE050 key ID: 0x00005002

App   :INFO :Inserted RSA certificate into SE050 key ID: 0x00005003

App   :INFO :RSA key written to DER, 294 bytes

App   :INFO :Inserted RSA public key into SE050 key ID: 0x00005004

App   :INFO :Inserted ECC private key into SE050 key ID: 0x00005005

App   :INFO :Inserted ECC certificate into SE050 key ID: 0x00005006

App   :INFO :Inserted ECC public key into SE050 key ID: 0x00005007

App   :INFO :Setting ecc_key to use ID from SE050

App   :INFO :Verifying ECDSA signature with SE050 key ID: 0x00005007

App   :INFO :ECDSA verify success!

App   :INFO :Read RSA certificate (1260 bytes)

App   :INFO :RSA cert matches original loaded

App   :INFO :Read ECC certificate (677 bytes)

App   :INFO :ECC cert matches original loaded

App   :INFO :Erased RSA private key, key ID: 0x00005002

App   :INFO :Erased RSA certificate, key ID: 0x00005003

App   :INFO :Erased RSA public key, key ID: 0x00005004

App   :INFO :Erased ECC private key, key ID: 0x00005005

App   :INFO :Erased ECC certificate, key ID: 0x00005006

App   :INFO :Erased ECC public key, key ID: 0x00005007

App   :INFO :Done with sample app
App   :INFO :ex_sss Finished
```

