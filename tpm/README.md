# TPM Examples

## EVP with TPM Example (evp_tpm.c)

This example shows use of the EVP compatibility layer with the crypto callbacks and TPM.
Note: Requires PR https://github.com/wolfSSL/wolfssl/pull/4333

### Building wolfSSL

```
% ./configure --enable-opensslextra --enable-wolftpm --enable-cryptocb [--enable-debug] [--prefix=DIR]
% make
% make install
```

### Building wolfTPM

```
% ./configure [--enable-swtpm] [--enable-debug] [--prefix=DIR]
% make
% make install
```

### Building evp_tpm example

```sh
% make
gcc -I/usr/local/include -Wall   -c -o evp_tpm.o evp_tpm.c
gcc -o evp_tpm evp_tpm.o -I/usr/local/include -Wall  -L/usr/local/lib -lwolfssl -lwolftpm
```

### Example Output

```sh
% ./evp_tpm
wolfSSL Entering wolfCrypt_Init
TPM2: Caps 0x00000000, Did 0x0000, Vid 0x0000, Rid 0x 0
TPM2_Startup pass
TPM2_SelfTest pass
TPM2_ReadPublic Handle 0x81000201: pub 90, name 34, qualifiedName 34
Loading SRK: Storage 0x81000201 (90 bytes)
TPM2_Create key: pub 88, priv 126
Public Area (size 88):
  Type: ECC (0x23), name: SHA256 (0xB), objAttr: 0x40460, authPolicy sz: 0
  ECC: sym algorithm: NULL (0x10), sym keyBits: 0, sym mode: Unknown (0x0)
       scheme: ECDSA (0x18), scheme hash: SHA256 (0xB), curveID: size 32, 0x3
       KDF scheme: NULL (0x10), KDF alg: Unknown (0x0), unique X/Y size 32/32
TPM2_Load Key Handle 0x80000002
Create/Load ECC Key: Handle 0x80000002 (88 bytes)
EVP_MD_ecc_signing()
CryptoCbFunc Pk: Type 4
TPM2_Sign: ECDSA 64
EVP_DigestSignFinal sz 72
EVP_DigestVerifyFinal success
Result: success (0)
TPM2_FlushContext: Closed handle 0x80000002
```

## Support

For questions please email support@wolfssl.com
