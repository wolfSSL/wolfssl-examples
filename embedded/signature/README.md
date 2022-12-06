# Signature Examples for Embedded Systems
​
This directory includes the following examples under the sub-directories.Each has a Makefile and source files to build and execute the example and a README to show how to build and Example output.
​
|Scheme|Directory|Description|
|---|---|---|
|RSA|rsa_vfy_only |verify signature|
||rsa_buffer|sign/verify signature |
|ECDSA|signature/ecc-sign-verify/ecc_sign_verify.c|sign msg and verify signature|

You can specify a target function of Simple example, Benchemark or Memory track program.It also has options for optimized code for MCU architectures such as Intel x86, ARM64 or a generic code by default, as well as Math library of Single Precision or TFM.

```
$ make <Function> math=<Mathlib> arch=<MCU>
```
​
## Functions

|Function name|Description|
|---|---|
|Default|Simple Execution|
|mem|Memory Track on heap and stack usage|
|bench|Performance benchmark|


## Math library
|math|Description|
|---|---|
|Default|Generic architecture by pure C language source code|
|sp| SP for specified archtecture|
|tfm|TFM for generic architecture|
## MCU Architectures
NOTE:  No architecture specification is required when using TFM.
|arch|Description|
|---|---|
|Default|Generic architecture by pure C language source code|
|arm64|SP for ARM64 |
|x64|SP for x86 64bit|


The Makefile is self-contained without libwolfssl. Put your wolfSSL source filesin parallel with wolfssl-examples directory. It is defined by WOLFROOT in Makefile.It compiles only needed files for the target. OBJ and OBJ_xxx macro in Makefiledefine object files for the common and specific target.
​
Example programs are hard coded for a hash algorithm or signature scheme.Sha256 is for the hash by default. PKCS #1 v1.5 or ECDSA for the signature scheme.You can refer to the following API tables for modifying the examples for other algorithms or schemes.
​
## Table 1: Hash algorithms for PKCS#1 Signature
|Algorithm|Src File|Macro SW<br>Enable|<br>Disable|Note|
|---|---|---|---|---|
|MD2|md2.c|WOLFSSL_MD2||Only for v1.5 Backward compatibility|
|MD5|md5.c||NO_MD5|Only for v1.5 Backward compatibility|
|SHA1|sha.c||NO_SHA|||SHA256|sha256.c||NO_SHA256|
||SHA384|sha512.c|WOLFSSL_SHA384||Disabled by default|
|SHA512|sha512.c|WOLFSSL_SHA512||Disabled by default|


## Table 2: Hash Algorithm APIs
|Algorithm|<br>Init|API<br>Update|<br>Final|
|---|---|---|---|
|MD2|wc_InitMd2|wc_Md2Update|wc_Md2Final|
|MD5|wc_InitMd5|wc_Md5Update|wc_Md5Final|
|SHA1|wc_InitSha|wc_ShaUpdate|wc_ShaFinal|
|SHA256|wc_InitSha256|wc_Sha256Update|wc_Sha256Final|
|SHA384|wc_initSha384|wc_Sha384Update|wc_Sha384Final|
|SHA512|wc_InitSha512|wc_Sha512Update|wc_Sha512Final|

​
## Table 3: RSA Signature APIs
​
|Padding|API|Description|
|---|---|---|
|PKCS #1 v1.5|wc_RsaSSL_Verify|Decrypt input signature to verify|
||wc_RsaSSL_VerifyInline|The output uses the same byte array as the input|
|PSS|wc_RsaPSS_Verify|Decrypt input signature to verify with PSS|
| |wc_RsaPSS_VerifyCheck|Verify the message signed|
| |wc_RsaPSS_VerifyCheck_ex|with Salt length argument|
| |wc_RsaPSS_VerifyInline|The output uses the same byte array as the input|
| |wc_RsaPSS_VerifyCheckInline|Verify the message signed|
| |wc_RsaPSS_VerifyCheckPadding|Checks the PSS data to ensure that the signature matches|
| |wc_RsaPSS_VerifyCheckPadding_ex|with Salt length argument|


## Table 4: ECC Signature APIs
​
|Algorithm|API|Hash|
|---|---|---|
|ECDSA|wc_ecc_sign_hash|SHA512|
|Ed25519|wc_ed25519_sign_hash|SHA512|
|Ed488|wc_ed488_sign_hash|SHAKE256|
