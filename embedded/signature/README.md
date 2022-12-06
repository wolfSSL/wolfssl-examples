# Signature Examples and APIs
"sigunature" directry includes following examples in the table.

|Scheme|Source|Description|Benchmark|Memory track|
|---|---|---|:---:|:---:|
|RSA|signature/rsa_vfy_only/verify.c |verify signature|O|O
||signature/rsa_buffer/sign.c|sign input message|X|X|
||signature/rsa_buffer/verify.c|verify signature |O|O|
|ECDSA|signature/ecc-sign-verify/ecc_sign_verify.c|sign msg and verify signature|O|O|



## Table 1: Hash algorithms for PKCS#1 Signature

|Algorithm|Src File|Macro SW<br>Enable|<br>Disable|Note|
|---|---|---|---|---|
|MD2|md2.c|WOLFSSL_MD2||Only for v1.5 Backword compatibility|
|MD5|md5.c||NO_MD5|Only for v1.5 Backword compatibility|
|SHA1|sha.c||NO_SHA||
|SHA256|sha256.c||NO_SHA256||
|SHA384|sha512.c|WOLFSSL_SHA384||Disabled by default|
|SHA512|sha512.c|WOLFSSL_SHA512||Disabled by default|


## Table 2: Hash Algorithm APIs​​

|​Algorithm|<br>Init|API<br>Update|<br>Final|
|---|---|---|---|
|MD2|wc_InitMd2|wc_Md2Update|wc_Md2Final|
|MD5|wc_InitMd5|wc_Md5Update|wc_Md5Final|
|SHA1|wc_InitSha|wc_ShaUpdate|wc_ShaFinal|
|SHA256|wc_InitSha256|wc_Sha256Update|wc_Sha256Final|
|SHA384|wc_initSha384|wc_Sha384Update|wc_Sha384Final|
|SHA512|wc_InitSha512|wc_Sha512Update|wc_Sha512Final|

## Table 3: RSA Signature APIs
​
|Padding|API|Description|
|---|---|---|
|PKCS #1 v1.5|wc_RsaSSL_Verify|Decrypt input signature to verify|
||wc_RsaSSL_VerifyInline|The output uses the same byte array as the input|
|PSS|wc_RsaPSS_Verify|Decrypt input signature to verify with PSS|
|   |wc_RsaPSS_VerifyCheck|Verify the message signed|
|   |wc_RsaPSS_VerifyCheck_ex|with Salt length argument|
|   |wc_RsaPSS_VerifyInline|The output uses the same byte array as the input|
|   |wc_RsaPSS_VerifyCheckInline|Verify the message signed|
|   |wc_RsaPSS_VerifyCheckPadding|Checks the PSS data to ensure that the signature matches|
|   |wc_RsaPSS_VerifyCheckPadding_ex|with Salt length argument|

## ​Table 4: ECC Signature APIs
​
|Algorithm|API|Hash|
|---|---|---|
|ECDSA|wc_ecc_sign_hash|SHA512|
|Ed25519|wc_ed25519_sign_hash|SHA512|
|Ed488|wc_ed488_sign_hash|SHAKE256|


