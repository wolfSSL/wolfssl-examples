# wolfCrypt SE050 Certificate Signing Request (CSR) Example

This example uses wolfSSL to generate a Certificate Signing Request (CSR) using
a key that is generated and stored in the SE050:

1. Generates a 256 bit ECC key pair in SE050.

2. Generates a certificate request with wolfCrypt and signs it with the
   ECC private key from step 1.

3. Converts the CSR from DER to PEM format and prints that to the console.

4. Deletes the generated key in SE050 (for example cleanup), and cleans up
   wolfSSL library resources.

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
$ ./wolfcrypt_generate_csr
```

On successful run, output similar to the following will print out:

```
App   :INFO :PlugAndTrust_v04.02.00_20220524
App   :INFO :Running ./wolfcrypt_generate_csr
App   :INFO :If you want to over-ride the selection, use ENV=EX_SSS_BOOT_SSS_PORT or pass in command line arguments.
App   :INFO :Using default PlatfSCP03 keys. You can use keys from file using ENV=EX_SSS_BOOT_SCP03_PATH
sss   :INFO :atr (Len=35)
      00 A0 00 00    03 96 04 03    E8 00 FE 02    0B 03 E8 08
      01 00 00 00    00 64 00 00    0A 4A 43 4F    50 34 20 41
      54 50 4F
App   :INFO :Running wc_se050_set_config()
App   :INFO :SE050 config successfully set in wolfSSL
App   :INFO :wolfCrypt example of CSR generation

App   :INFO :Initializing RNG

App   :INFO :Generating ECC key in SE050

sss   :WARN :nxEnsure:'ret == SM_OK' failed. At Line:6971 Function:sss_se05x_TXn
sss   :WARN :Could not delete Key id FFFF
App   :INFO :Generated ECC key in SE050 ID: 0x0000ffff

App   :INFO :Generated CSR (530 bytes)

App   :INFO :-----BEGIN CERTIFICATE REQUEST-----
MIIBTjCB8wIBAjCBkDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAk1UMRAwDgYDVQQH
DAdCb3plbWFuMREwDwYDVQQKDAhUZXN0IE9yZzEUMBIGA1UECwwLRGV2ZWxvcG1l
bnQxGDAWBgNVBAMMD3d3dy5leGFtcGxlLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5m
b0BleGFtcGxlLmNvbTBZMBM
App   :INFO :Erasing ECC key stored in SE050 slot: 0x0000ffff

App   :INFO :Done with sample app
App   :INFO :ex_sss Finished
```

