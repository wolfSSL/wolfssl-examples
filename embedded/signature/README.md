# Signature Examples for Embedded Systems
​
This directory includes the following examples. Each subdirectory has a Makefile, source files, and a README to show how to build the example, along with expected example output.
​
|Scheme|Directory|Description|
|---|---|---|
|RSA|rsa_sign_verify|sign/verify signature inline  |
||rsa_vfy_only |verify signature|
||rsa_vfy_only_nonblock|verify signature with non-blocking|
|ECDSA|ecc_sign_verify|sign msg and verify signature|
||ecc_vfy_only|verify Signature|
||ecc_vfy_only_nonblock|verify signature with non-blocking|


When building each example, you can specify arguments to control the build.  Specify a target function to run either a simple example, benchmark, or memory tracking example. Specify an "arch" option to build optimized code for MCU architectures such as Intel x86, ARM64 or a generic code by default.  And specify a "math" option to choose an underlying wolfCrypt math library to use, between Single Precision or TFM.


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
|sp| SP for generic or specified  architecture|
|tfm|TFM for generic architecture|
## MCU Architectures
NOTE:  No architecture specification is required when using TFM.
|arch|Description|
|---|---|
|Default|Generic architecture by pure C language source code|
|c32| SP using 32-bit data type |
|c64| SP using 64-bit data type   (default)   |
|arm64|SP for ARM64 |
|x64|SP for x86 64bit|


Each Makefile is self-contained to statically link wolfCrypt source files (without using a shared libwolfssl.so). Put your wolfSSL source files in parallel with the wolfssl-examples directory. The location of the primary wolfSSL source directory is defined by WOLFROOT in each Makefile. Each build compiles only the needed files for the target. OBJ and OBJ_xxx macros in each Makefile define object files for the common and specific target.
​
Example programs are hard coded to use a specific hash algorithm or signature scheme. Sha256 is used for the hash by default. PKCS#1 v1.5 or ECDSA is used for the signature scheme. You can refer to the following API tables for modifying the examples for other algorithms or schemes.

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
