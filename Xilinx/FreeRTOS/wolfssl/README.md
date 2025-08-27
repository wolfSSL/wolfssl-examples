# Notes - Please read

## Note 1
```
wolfSSL as of 3.6.6 no longer enables SSLv3 by default.  wolfSSL also no
longer supports static key cipher suites with PSK, RSA, or ECDH.  This means
if you plan to use TLS cipher suites you must enable DH (DH is on by default),
or enable ECC (ECC is on by default on 64bit systems), or you must enable static
key cipher suites with
    WOLFSSL_STATIC_DH
    WOLFSSL_STATIC_RSA
    or
    WOLFSSL_STATIC_PSK

though static key cipher suites are deprecated and will be removed from future
versions of TLS.  They also lower your security by removing PFS.  Since current
NTRU suites available do not use ephemeral keys, WOLFSSL_STATIC_RSA needs to be
used in order to build with NTRU suites.


When compiling ssl.c, wolfSSL will now issue a compiler error if no cipher suites
are available.  You can remove this error by defining WOLFSSL_ALLOW_NO_SUITES
in the event that you desire that, i.e., you're not using TLS cipher suites.
```

## Note 2
```

wolfSSL takes a different approach to certificate verification than OpenSSL
does.  The default policy for the client is to verify the server, this means
that if you don't load CAs to verify the server you'll get a connect error,
no signer error to confirm failure (-188).  If you want to mimic OpenSSL
behavior of having SSL_connect succeed even if verifying the server fails and
reducing security you can do this by calling:

wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

before calling wolfSSL_new();  Though it's not recommended.
```

# wolfSSL (Formerly CyaSSL) Release 3.12.2 (10/23/2017)

## Release 3.12.2 of wolfSSL has bug fixes and new features including:

This release includes many performance improvements with Intel ASM (AVX/AVX2) and AES-NI. New single precision math option to speedup RSA, DH and ECC. Embedded hardware support has been expanded for STM32, PIC32MZ and ATECC508A. AES now supports XTS mode for disk encryption. Certificate improvements for setting serial number, key usage and extended key usage. Refactor of SSL_ and hash types to allow openssl coexistence. Improvements for TLS 1.3. Fixes for OCSP stapling to allow disable and WOLFSSL specific user context for callbacks. Fixes for openssl and MySQL compatibility. Updated Micrium port. Fixes for asynchronous modes.

* Added TLS extension for Supported Point Formats (ec_point_formats)
* Fix to not send OCSP stapling extensions in client_hello when not enabled
* Added new API's for disabling OCSP stapling
* Add check for SIZEOF_LONG with sun and LP64
* Fixes for various TLS 1.3 disable options (RSA, ECC and ED/Curve 25519).
* Fix to disallow upgrading to TLS v1.3
* Fixes for wolfSSL_EVP_CipherFinal() when message size is a round multiple of a block size.
* Add HMAC benchmark and expanded AES key size benchmarks
* Added simple GCC ARM Makefile example
* Add tests for 3072-bit RSA and DH.
* Fixed DRAFT_18 define and fixed downgrading with TLS v1.3
* Fixes to allow custom serial number during certificate generation
* Add method to get WOLFSSL_CTX certificate manager
* Improvement to `wolfSSL_SetOCSP_Cb` to allow context per WOLFSSL object
* Alternate certificate chain support `WOLFSSL_ALT_CERT_CHAINS`. Enables checking cert against multiple CA's. 
* Added new `--disable-oldnames` option to allow for using openssl along-side wolfssl headers (without OPENSSL_EXTRA).
* Refactor SSL_ and hashing types to use wolf specific prefix (WOLFSSL and WC_) to allow openssl coexistence.
* Fixes for HAVE_INTEL_MULX
* Cleanup include paths for MySQL cmake build
* Added configure option for building library for wolfSSH (--enable-wolfssh)
* Openssl compatibility layer improvements
* Expanded API unit tests
* Fixes for STM32 crypto hardware acceleration
* Added AES XTS mode (--enable-xts)
* Added ASN Extended Key Usage Support (see wc_SetExtKeyUsage).
* Math updates and added TFM_MIPS speedup.
* Fix for creation of the KeyUsage BitString
* Fix for 8k keys with MySQL compatibility
* Fixes for ATECC508A.
* Fixes for PIC32MZ hashing.
* Fixes and improvements to asynchronous modes for Intel QuickAssist and Cavium Nitrox V.
* Update HASH_DRBG Reseed mechanism and add test case
* Rename the file io.h/io.c to wolfio.h/wolfio.c
* Cleanup the wolfIO_Send function.
* OpenSSL Compatibility Additions and Fixes
* Improvements to Visual Studio DLL project/solution.
* Added function to generate public ECC key from private key
* Added async blocking support for sniffer tool.
* Added wolfCrypt hash tests for empty string and large data.
* Added ability to use of wolf implementation of `strtok` using `USE_WOLF_STRTOK`.
* Updated Micrium uC/OS-III Port
* Updated root certs for OCSP scripts
* New Single Precision math option for RSA, DH and ECC (off by default). See `--enable-sp`.
* Speedups for AES GCM with AESNI (--enable-aesni)
* Speedups for SHA2, ChaCha20/Poly1035 using AVX/AVX2


# wolfSSL (Formerly CyaSSL) Release 3.12.0 (8/04/2017)

## Release 3.12.0 of wolfSSL has bug fixes and new features including:

- TLS 1.3 with Nginx! TLS 1.3 with ARMv8! TLS 1.3 with Async Crypto! (--enable-tls13)
- TLS 1.3 0RTT feature added
- Added port for using Intel SGX with Linux
- Update and fix PIC32MZ port
- Additional unit testing for MD5, SHA, SHA224, SHA256, SHA384, SHA512, RipeMd, HMAC, 3DES, IDEA, ChaCha20, ChaCha20Poly1305 AEAD, Camellia, Rabbit, ARC4, AES, RSA, Hc128
- AVX and AVX2 assembly for improved ChaCha20 performance
- Intel QAT fixes for when using --disable-fastmath
- Update how DTLS handles decryption and MAC failures
- Update DTLS session export version number for --enable-sessionexport feature
- Add additional input argument sanity checks to ARMv8 assembly port
- Fix for making PKCS12 dynamic types match
- Fixes for potential memory leaks when using --enable-fast-rsa
- Fix for when using custom ECC curves and add BRAINPOOLP256R1 test
- Update TI-RTOS port for dependency on new wolfSSL source files
- DTLS multicast feature added, --enable-mcast
- Fix for Async crypto with GCC 7.1 and HMAC when not using Intel QuickAssist
- Improvements and enhancements to Intel QuickAssist support
- Added Xilinx port
- Added SHA3 Keccak feature, --enable-sha3
- Expand wolfSSL Python wrapper to now include a client side implementation
- Adjust example servers to not treat a peer closed error as a hard error
- Added more sanity checks to fp_read_unsigned_bin function
- Add SHA224 and AES key wrap to ARMv8 port
- Update MQX classics and mmCAU ports
- Fix for potential buffer over read with wolfSSL_CertPemToDer
- Add PKCS7/CMS decode support for KARI with IssuerAndSerialNumber
- Fix ThreadX/NetX warning
- Fixes for OCSP and CRL non blocking sockets and for incomplete cert chain with OCSP
- Added RSA PSS sign and verify
- Fix for STM32F4 AES-GCM
- Added enable all feature (--enable-all)
- Added trackmemory feature (--enable-trackmemory)
- Fixes for AES key wrap and PKCS7 on Windows VS
- Added benchmark block size argument
- Support use of staticmemory with PKCS7
- Fix for Blake2b build with GCC 5.4
- Fixes for compiling wolfSSL with GCC version 7, most dealing with switch statement fall through warnings.
- Added warning when compiling without hardened math operations


Note:
There is a known issue with using ChaCha20 AVX assembly on versions of GCC earlier than 5.2. This is encountered with using the wolfSSL enable options --enable-intelasm and --enable-chacha. To avoid this issue ChaCha20 can be enabled with --enable-chacha=noasm.
If using --enable-intelasm and also using --enable-sha224 or --enable-sha256 there is a known issue with trying to use -fsanitize=address.

This release of wolfSSL fixes 1 low level security vulnerability.

Low level fix for a potential DoS attack on a wolfSSL client. Previously a client would accept many warning alert messages without a limit. This fix puts a limit to the number of warning alert messages received and if this limit is reached a fatal error ALERT_COUNT_E is returned. The max number of warning alerts by default is set to 5 and can be adjusted with the macro WOLFSSL_ALERT_COUNT_MAX. Thanks for the report from Tarun Yadav and Koustav Sadhukhan from Defence Research and Development Organization, INDIA.


See INSTALL file for build instructions.
More info can be found on-line at http://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.11.1 (5/11/2017)

## Release 3.11.1 of wolfSSL is a TLS 1.3 BETA release, which includes:

- TLS 1.3 client and server support for TLS 1.3 with Draft 18 support

This is strictly a BETA release, and designed for testing and user feedback.
Please send any comments, testing results, or feedback to wolfSSL at
support@wolfssl.com.

See INSTALL file for build instructions.
More info can be found on-line at http://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.11.0 (5/04/2017)

## Release 3.11.0 of wolfSSL has bug fixes and new features including:

- Code updates for warnings reported by Coverity scans
- Testing and warning fixes for FreeBSD on PowerPC
- Updates and refactoring done to ASN1 parsing functions
- Change max PSK identity buffer to account for an identity length of 128 characters
- Update Arduino script to handle recent files and additions
- Added support for PKCS#7 Signed Data with ECDSA
- Fix for interoperability with ChaCha20-Poly1305 suites using older draft versions
- DTLS update to allow multiple handshake messages in one DTLS record. Thanks to Eric Samsel over at Welch Allyn for reporting this bug.
- Intel QuickAssist asynchronous support (PR #715 - https://www.wolfssl.com/wolfSSL/Blog/Entries/2017/1/18_wolfSSL_Asynchronous_Intel_QuickAssist_Support.html)
- Added support for HAproxy load balancer
- Added option to allow SHA1 with TLS 1.2 for IIS compatibility (WOLFSSL_ALLOW_TLS_SHA1)
- Added Curve25519 51-bit Implementation, increasing performance on systems that have 128 bit types
- Fix to not send session ID on server side if session cache is off unless we're echoing 
session ID as part of session tickets
- Fixes for ensuring all default ciphers are setup correctly (see PR #830)
- Added NXP Hexiwear example in `IDE/HEXIWEAR`.
- Added wolfSSL_write_dup() to create write only WOLFSSL object for concurrent access
- Fixes for TLS elliptic curve selection on private key import.
- Fixes for RNG with Intel rdrand and rdseed speedups.
- Improved performance with Intel rdrand to use full 64-bit output
- Added new --enable-intelrand option to indicate use of RDRAND preference for RNG source 
- Removed RNG ARC4 support
- Added ECC helpers to get size and id from curve name.
- Added ECC Cofactor DH (ECC-CDH) support
- Added ECC private key only import / export functions.
- Added PKCS8 create function
- Improvements to TLS layer CTX handling for switching keys / certs.
- Added check for duplicate certificate policy OID in certificates.
- Normal math speed-up to not allocate on mp_int and defer until mp_grow
- Reduce heap usage with fast math when not using ALT_ECC_SIZE
- Fixes for building CRL with Windows
- Added support for inline CRL lookup when HAVE_CRL_IO is defined
- Added port for tenAsys INtime RTOS
- Improvements to uTKernel port (WOLFSSL_uTKERNEL2)
- Updated WPA Supplicant support
- Added support for Nginx
- Update stunnel port for version 5.40
- Fixes for STM32 hardware crypto acceleration
- Extended test code coverage in bundled test.c
- Added a sanity check for minimum authentication tag size with AES-GCM. Thanks to Yueh-Hsun Lin and Peng Li at KNOX Security at Samsung Research America for suggesting this.
- Added a sanity check that subject key identifier is marked as non-critical and a check that no policy OIDS appear more than once in the cert policies extension. Thanks to the report from Professor Zhenhua Duan, Professor Cong Tian, and Ph.D candidate Chu Chen from Institute of Computing Theory and Technology (ICTT) of Xidian University, China. Profs. Zhenhua Duan and Cong Tian are supervisors of Ph.D candidate Chu Chen.

This release of wolfSSL fixes 5 low and 1 medium level security vulnerability.

3 Low level fixes reported by Yueh-Hsun Lin and Peng Li from KNOX Security, Samsung Research America.
- Fix for out of bounds memory access in wc_DhParamsLoad() when GetLength() returns a zero. Before this fix there is a case where wolfSSL would read out of bounds memory in the function wc_DhParamsLoad.
- Fix for DH key accepted by wc_DhAgree when the key was malformed.
- Fix for a double free case when adding CA cert into X509_store.

Low level fix for memory management with static memory feature enabled. By default static memory is disabled. Thanks to GitHub user hajjihraf for reporting this.


Low level fix for out of bounds write in the function wolfSSL_X509_NAME_get_text_by_NID. This function is not used by TLS or crypto operations but could result in a buffer out of bounds write by one if called explicitly in an application. Discovered by Aleksandar Nikolic of Cisco Talos. http://talosintelligence.com/vulnerability-reports/

Medium level fix for check on certificate signature. There is a case in release versions 3.9.10, 3.10.0 and 3.10.2 where a corrupted signature on a peer certificate would not be properly flagged. Thanks to Wens Lo, James Tsai, Kenny Chang, and Oscar Yang at Castles Technology.


See INSTALL file for build instructions.
More info can be found on-line at http://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.10.2 (2/10/2017)

## Release 3.10.2 of wolfSSL has bug fixes and new features including:

- Poly1305 Windows macros fix. Thanks to GitHub user Jay Satiro
- Compatibility layer expanded with multiple functions added
- Improve fp_copy performance with ALT_ECC_SIZE
- OCSP updates and improvements
- Fixes for IAR EWARM 8 compiler warnings
- Reduce stack usage with ECC_CACHE_CURVE disabled
- Added ECC export raw for public and private key
- Fix for NO_ASN_TIME build
- Supported curves extensions now populated by default
- Add DTLS build without big integer math
- Fix for static memory feature with wc_ecc_verify_hash_ex and not SHAMIR
- Added PSK interoperability testing to script bundled with wolfSSL
- Fix for Python wrapper random number generation. Compiler optimizations with Python could place the random number in same buffer location each time. Thanks to GitHub user Erik Bray (embray)
- Fix for tests on unaligned memory with static memory feature
- Add macro WOLFSSL_NO_OCSP_OPTIONAL_CERTS to skip optional OCSP certificates
- Sanity checks on NULL arguments added to wolfSSL_set_fd and wolfSSL_DTLS_SetCookieSecret
- mp_jacobi stack use reduced, thanks to Szabi Tolnai for providing a solution to reduce stack usage


This release of wolfSSL fixes 2 low and 1 medium level security vulnerability.

Low level fix of buffer overflow for when loading in a malformed temporary DH file. Thanks to Yueh-Hsun Lin and Peng Li from KNOX Security, Samsung Research America for the report.

Medium level fix for processing of OCSP response. If using OCSP without hard faults enforced and no alternate revocation checks like OCSP stapling then it is recommended to update.

Low level fix for potential cache attack on RSA operations. If using wolfSSL RSA on a server that other users can have access to monitor the cache, then it is recommended to update wolfSSL. Thanks to Andreas Zankl, Johann Heyszl and Georg Sigl at Fraunhofer AISEC for the initial report.

See INSTALL file for build instructions.
More info can be found on-line at http://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.10.0 (12/21/2016)

## Release 3.10.0 of wolfSSL has bug fixes and new features including:

- Added support for SHA224
- Added scrypt feature
- Build for Intel SGX use, added in directory IDE/WIN-SGX
- Fix for ChaCha20-Poly1305 ECDSA certificate type request
- Enhance PKCS#7 with ECC enveloped data and AES key wrap support
- Added support for RIOT OS
- Add support for parsing PKCS#12 files
- ECC performance increased with custom curves
- ARMv8 expanded to AArch32 and performance increased
- Added ANSI-X9.63-KDF support
- Port to STM32 F2/F4 CubeMX
- Port to Atmel ATECC508A board
- Removed fPIE by default when wolfSSL library is compiled
- Update to Python wrapper, dropping DES and adding wc_RSASetRNG
- Added support for NXP K82 hardware acceleration
- Added SCR client and server verify check
- Added a disable rng option with autoconf
- Added more tests vectors to test.c with AES-CTR
- Updated DTLS session export version number
- Updated DTLS for 64 bit sequence numbers
- Fix for memory management with TI and WOLFSSL_SMALL_STACK
- Hardening RSA CRT to be constant time
- Fix uninitialized warning with IAR compiler
- Fix for C# wrapper example IO hang on unexpected connection termination


This release of wolfSSL fixes a low level security vulnerability. The vulnerability reported was a potential cache attack on RSA operations. If using wolfSSL RSA on a server that other users can have access to monitor the cache, then it is recommended to update wolfSSL. Thanks to Andreas Zankl, Johann Heyszl and Georg Sigl at Fraunhofer AISEC for the report. More information will be available on our site:

https://wolfssl.com/wolfSSL/security/vulnerabilities.php

See INSTALL file for build instructions.
More info can be found on-line at http://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.9.10 (9/23/2016)

## Release 3.9.10 of wolfSSL has bug fixes and new features including:

- Default configure option changes:
  1. DES3 disabled by default
  2. ECC Supported Curves Extension enabled by default
  3. New option Extended Master Secret enabled by default
- Added checking CA certificate path length, and new test certs
- Fix to DSA pre padding and sanity check on R/S values
- Added CTX level RNG for single-threaded builds
- Intel RDSEED enhancements
- ARMv8 hardware acceleration support for AES-CBC/CTR/GCM, SHA-256
- Arduino support updates
- Added the Extended Master Secret TLS extension
  1. Enabled by default in configure options, API to disable
  2. Added support for Extended Master Secret to sniffer
- OCSP fix with issuer key hash, lookup refactor
- Added support for Frosted OS
- Added support for DTLS over SCTP
- Added support for static memory with wolfCrypt
- Fix to ECC Custom Curve support
- Support for asynchronous wolfCrypt RSA and TLS client
- Added distribution build configure option
- Update the test certificates

This release of wolfSSL fixes medium level security vulnerabilities.  Fixes for
potential AES, RSA, and ECC side channel leaks is included that a local user
monitoring the same CPU core cache could exploit.  VM users, hyper-threading
users, and users where potential attackers have access to the CPU cache will
need to update if they utilize AES, RSA private keys, or ECC private keys.
Thanks to Gorka Irazoqui Apecechea and Xiaofei Guo from Intel Corporation for
the report.  More information will be available on our site:

https://wolfssl.com/wolfSSL/security/vulnerabilities.php

See INSTALL file for build instructions.
More info can be found on-line at https://wolfssl.com/wolfSSL/Docs.html


# wolfSSL (Formerly CyaSSL) Release 3.9.8 (7/29/2016)

##Release 3.9.8 of wolfSSL has bug fixes and new features including:

- Add support for custom ECC curves.
- Add cipher suite ECDHE-ECDSA-AES128-CCM.
- Add compkey enable option. This option is for compressed ECC keys.
- Add in the option to use test.h without gettimeofday function using the macro
  WOLFSSL_USER_CURRTIME.
- Add RSA blinding for private key operations. Enable option of harden which is
  on by default. This negates timing attacks.
- Add ECC and TLS support for all SECP, Koblitz and Brainpool curves.
- Add helper functions for static memory option to allow getting optimum buffer
  sizes.
- Update DTLS behavior on bad MAC. DTLS silently drops packets with bad MACs now.
- Update fp_isprime function from libtom enchancement/cleanup repository.
- Update sanity checks on inputs and return values for AES-CMAC.
- Update wolfSSL for use with MYSQL v5.6.30.
- Update LPCXpresso eclipse project to not include misc.c when not needed.
- Fix retransmit of last DTLS flight with timeout notification. The last flight
  is no longer retransmitted on timeout.
- Fixes to some code in math sections for compressed ECC keys. This includes
  edge cases for buffer size on allocation and adjustments for compressed curves
  build. The code and full list can be found on github with pull request #456.
- Fix function argument mismatch for build with secure renegotiation.
- X.509 bug fixes for reading in malformed certificates, reported by researchers
  at Columbia University
- Fix GCC version 6 warning about hard tabs in poly1305.c. This was a warning
  produced by GCC 6 trying to determine the intent of code.
- Fixes for static memory option. Including avoid potential race conditions with
  counters, decrement handshake counter correctly.
- Fix anonymous cipher with Diffie Hellman on the server side. Was an issue of a
  possible buffer corruption. For information and code see pull request #481.


- One high level security fix that requires an update for use with static RSA
  cipher suites was submitted. This fix was the addition of RSA blinding for
  private RSA operations. We recommend servers who allow static RSA cipher
  suites to also generate new private RSA keys. Static RSA cipher suites are
  turned off by default.

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/wolfSSL/Docs.html

# wolfSSL (Formerly CyaSSL) Release 3.9.6 (6/14/2016)

##Release 3.9.6 of wolfSSL has bug fixes and new features including:

- Add staticmemory feature
- Add public wc_GetTime API with base64encode feature
- Add AES CMAC algorithm
- Add DTLS sessionexport feature
- Add python wolfCrypt wrapper
- Add ECC encrypt/decrypt benchmarks
- Add dynamic session tickets
- Add eccshamir option
- Add Whitewood netRandom support --with-wnr
- Add embOS port
- Add minimum key size checks for RSA and ECC
- Add STARTTLS support to examples
- Add uTasker port
- Add asynchronous crypto and wolf event support
- Add compile check for misc.c with inline
- Add RNG benchmark
- Add reduction to stack usage with hash-based RNG
- Update STM32F2_CRYPTO port with additional algorithms supported
- Update MDK5 projects
- Update AES-NI
- Fix for STM32 with STM32F2_HASH defined
- Fix for building with MinGw
- Fix ECC math bugs with ALT_ECC_SIZE and key sizes over 256 bit (1)
- Fix certificate buffers github issue #422
- Fix decrypt max size with RSA OAEP
- Fix DTLS sanity check with DTLS timeout notification
- Fix free of WOLFSSL_METHOD on failure to create CTX
- Fix memory leak in failure case with wc_RsaFunction (2)

- No high level security fixes that requires an update though we always
recommend updating to the latest
- (1) Code changes for ECC fix can be found at pull requests #411, #416, and #428
- (2) Builds using RSA with using normal math and not RSA_LOW_MEM should update

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/wolfSSL/Docs.html

# wolfSSL (Formerly CyaSSL) Release 3.9.0 (03/18/2016)

##Release 3.9.0 of wolfSSL has bug fixes and new features including:

- Add new leantls configuration
- Add RSA OAEP padding at wolfCrypt level
- Add Arduino port and example client
- Add fixed point DH operation
- Add CUSTOM_RAND_GENRATE_SEED_OS and CUSTOM_RAND_GENERATE_BLOCK
- Add ECDHE-PSK cipher suites
- Add PSK ChaCha20-Poly1305 cipher suites
- Add option for fail on no peer cert except PSK suites
- Add port for Nordic nRF51
- Add additional ECC NIST test vectors for 256, 384 and 521
- Add more granular ECC, Ed25519/Curve25519 and AES configs
- Update to ChaCha20-Poly1305
- Update support for Freescale KSDK 1.3.0
- Update DER buffer handling code, refactoring and reducing memory
- Fix to AESNI 192 bit key expansion
- Fix to C# wrapper character encoding
- Fix sequence number issue with DTLS epoch 0 messages
- Fix RNGA with K64 build
- Fix ASN.1 X509 V3 certificate policy extension parsing
- Fix potential free of uninitialized RSA key in asn.c
- Fix potential underflow when using ECC build with FP_ECC
- Fixes for warnings in Visual Studio 2015 build

- No high level security fixes that requires an update though we always
recommend updating to the latest
- FP_ECC is off by default, users with it enabled should update for the zero
sized hash fix

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html

# wolfSSL (Formerly CyaSSL) Release 3.8.0 (12/30/2015)

##Release 3.8.0 of wolfSSL has bug fixes and new features including:

- Example client/server with VxWorks
- AESNI use with AES-GCM
- Stunnel compatibility enhancements
- Single shot hash and signature/verify API added
- Update cavium nitrox port
- LPCXpresso IDE support added
- C# wrapper to support wolfSSL use by a C# program
- (BETA version)OCSP stapling added
- Update OpenSSH compatibility
- Improve DTLS handshake when retransmitting finished message
- fix idea_mult() for 16 and 32bit systems
- fix LowResTimer on Microchip ports

- No high level security fixes that requires an update though we always
recommend updating to the latest

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html

# wolfSSL (Formerly CyaSSL) Release 3.7.0 (10/26/2015)

##Release 3.7.0 of wolfSSL has bug fixes and new features including:

- ALPN extension support added for HTTP2 connections with --enable-alpn
- Change of example/client/client max fragment flag -L -> -F
- Throughput benchmarking, added scripts/benchmark.test
- Sniffer API ssl_FreeDecodeBuffer added
- Addition of AES_GCM to Sniffer
- Sniffer change to handle unlimited decrypt buffer size
- New option for the sniffer where it will try to pick up decoding after a
  sequence number acknowldgement fault. Also includes some additional stats.
- JNI API setter and getter function for jobject added
- User RSA crypto plugin abstraction. An example placed in wolfcrypt/user-crypto
- fix to asn configuration bug
- AES-GCM/CCM fixes.
- Port for Rowley added
- Rowley Crossworks bare metal examples added
- MDK5-ARM project update
- FreeRTOS support updates.
- VXWorks support updates.
- Added the IDEA cipher and support in wolfSSL.
- Update wolfSSL website CA.
- CFLAGS is usable when configuring source.

- No high level security fixes that requires an update though we always
recommend updating to the latest

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html

#wolfSSL (Formerly CyaSSL) Release 3.6.8 (09/17/2015)

##Release 3.6.8 of wolfSSL fixes two high severity vulnerabilities.
##It also includes bug fixes and new features including:

- Two High level security fixes, all users SHOULD update.
  a) If using wolfSSL for DTLS on the server side of a publicly accessible
     machine you MUST update.
  b) If using wolfSSL for TLS on the server side with private RSA keys allowing
     ephemeral key exchange without low memory optimziations you MUST update and
     regenerate the private RSA keys.

     Please see https://www.wolfssl.com/wolfSSL/Blog/Blog.html for more details

- No filesystem build fixes for various configurations
- Certificate generation now supports several extensions including KeyUsage,
    SKID, AKID, and Ceritifcate Policies
- CRLs can be loaded from buffers as well as files now
- SHA-512 Ceritifcate Signing generation
- Fixes for sniffer reassembly processing

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html

#wolfSSL (Formerly CyaSSL) Release 3.6.6 (08/20/2015)

##Release 3.6.6 of wolfSSL has bug fixes and new features including:

- OpenSSH  compatibility with --enable-openssh
- stunnel  compatibility with --enable-stunnel
- lighttpd compatibility with --enable-lighty
- SSLv3 is now disabled by default, can be enabled with --enable-sslv3
- Ephemeral key cipher suites only are now supported by default
    To enable static ECDH cipher suites define WOLFSSL_STATIC_DH
    To enable static  RSA cipher suites define WOLFSSL_STATIC_RSA
    To enable static  PSK cipher suites define WOLFSSL_STATIC_PSK
- Added QSH (quantum-safe handshake) extension with --enable-ntru
- SRP is now part of wolfCrypt, enable with --enabe-srp
- Certificate handshake messages can now be sent fragmented if the record
  size is smaller than the total message size, no user action required.
- DTLS duplicate message fixes
- Visual Studio project files now support DLL and static builds for 32/64bit.
- Support for new Freesacle I/O
- FreeRTOS FIPS support

- No high level security fixes that requires an update though we always
  recommend updating to the latest

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html


#wolfSSL (Formerly CyaSSL) Release 3.6.0 (06/19/2015)

##Release 3.6.0 of wolfSSL has bug fixes and new features including:

- Max Strength build that only allows TLSv1.2, AEAD ciphers, and PFS (Perfect
   Forward Secrecy).  With --enable-maxstrength
- Server side session ticket support, the example server and echosever use the
   example callback myTicketEncCb(), see wolfSSL_CTX_set_TicketEncCb()
- FIPS version submitted for iOS.
- TI Crypto Hardware Acceleration
- DTLS fragmentation fixes
- ECC key check validation with wc_ecc_check_key()
- 32bit code options to reduce memory for Curve25519 and Ed25519
- wolfSSL JNI build switch with --enable-jni
- PicoTCP support improvements
- DH min ephemeral key size enforcement with wolfSSL_CTX_SetMinDhKey_Sz()
- KEEP_PEER_CERT and AltNames can now be used together
- ChaCha20 big endian fix
- SHA-512 signature algorithm support for key exchange and verify messages
- ECC make key crash fix on RNG failure, ECC users must update.
- Improvements to usage of time code.
- Improvements to VS solution files.
- GNU Binutils 2.24 ld has problems with some debug builds, to fix an ld error
  add -fdebug-types-section to C_EXTRA_FLAGS

- No high level security fixes that requires an update though we always
  recommend updating to the latest (except note 14, ecc RNG failure)

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html


#wolfSSL (Formerly CyaSSL) Release 3.4.8 (04/06/2015)

##Release 3.4.8 of wolfSSL has bug fixes and new features including:

- FIPS version submitted for iOS.
- Max Strength build that only allows TLSv1.2, AEAD ciphers, and PFS.
- Improvements to usage of time code.
- Improvements to VS solution files.

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html


#wolfSSL (Formerly CyaSSL) Release 3.4.6 (03/30/2015)

##Release 3.4.6 of wolfSSL has bug fixes and new features including:

- Intel Assembly Speedups using instructions rdrand, rdseed, aesni, avx1/2,
  rorx, mulx, adox, adcx .  They can be enabled with --enable-intelasm.
  These speedup the use of RNG, SHA2, and public key algorithms.
- Ed25519 support at the crypto level. Turn on with --enable-ed25519.  Examples
  in wolcrypt/test/test.c ed25519_test().
- Post Handshake Memory reductions.  wolfSSL can now hold less than 1,000 bytes
  of memory per secure connection including cipher state.
- wolfSSL API and wolfCrypt API fixes, you can still include the cyassl and
  ctaocrypt headers which will enable the compatibility APIs for the
  foreseeable future
- INSTALL file to help direct users to build instructions for their environment
- For ECC users with the normal math library a fix that prevents a crash when
  verify signature fails.  Users of 3.4.0 with ECC and the normal math library
  must update
- RC4 is now disabled by default in autoconf mode
- AES-GCM and ChaCha20/Poly1305 are now enabled by default to make AEAD ciphers
  available without a switch
- External ChaCha-Poly AEAD API, thanks to Andrew Burks for the contribution
- DHE-PSK cipher suites can now be built without ASN or Cert support
- Fix some NO MD5 build issues with optional features
- Freescale CodeWarrior project updates
- ECC curves can be individually turned on/off at build time.
- Sniffer handles Cert Status message and other minor fixes
- SetMinVersion() at the wolfSSL Context level instead of just SSL session level
  to allow minimum protocol version allowed at runtime
- RNG failure resource cleanup fix

- No high level security fixes that requires an update though we always
  recommend updating to the latest (except note 6 use case of ecc/normal math)

See INSTALL file for build instructions.
More info can be found on-line at //http://wolfssl.com/yaSSL/Docs.html


#wolfSSL (Formerly CyaSSL) Release 3.4.0 (02/23/2015)

## Release 3.4.0 wolfSSL has bug fixes and new features including:

- wolfSSL API and wolfCrypt API, you can still include the cyassl and ctaocrypt
  headers which will enable the compatibility APIs for the foreseeable future
- Example use of the wolfCrypt API can be found in wolfcrypt/test/test.c
- Example use of the wolfSSL API can be found in examples/client/client.c
- Curve25519 now supported at the wolfCrypt level, wolfSSL layer coming soon
- Improvements in the build configuration under AIX
- Microchip Pic32 MZ updates
- TIRTOS updates
- PowerPC updates
- Xcode project update
- Bidirectional shutdown examples in client/server with -w (wait for full
  shutdown) option
- Cycle counts on benchmarks for x86_64, more coming soon
- ALT_ECC_SIZE for reducing ecc heap use with fastmath when also using large RSA
  keys
- Various compile warnings
- Scan-build warning fixes
- Changed a memcpy to memmove in the sniffer (if using sniffer please update)
- No high level security fixes that requires an update though we always
  recommend updating to the latest


# CyaSSL Release 3.3.0 (12/05/2014)

- Countermeasuers for Handshake message duplicates, CHANGE CIPHER without
  FINISHED, and fast forward attempts.  Thanks to Karthikeyan Bhargavan from
  the Prosecco team at INRIA Paris-Rocquencourt for the report.
- FIPS version submitted
- Removes SSLv2 Client Hello processing, can be enabled with OLD_HELLO_ALLOWED
- User can set mimimum downgrade version with CyaSSL_SetMinVersion()
- Small stack improvements at TLS/SSL layer
- TLS Master Secret generation and Key Expansion are now exposed
- Adds client side Secure Renegotiation, * not recommended *
- Client side session ticket support, not fully tested with Secure Renegotiation
- Allows up to 4096bit DHE at TLS Key Exchange layer
- Handles non standard SessionID sizes in Hello Messages
- PicoTCP Support
- Sniffer now supports SNI Virtual Hosts
- Sniffer now handles non HTTPS protocols using STARTTLS
- Sniffer can now parse records with multiple messages
- TI-RTOS updates
- Fix for ColdFire optimized fp_digit read only in explicit 32bit case
- ADH Cipher Suite ADH-AES128-SHA for EAP-FAST

The CyaSSL manual is available at:
http://www.wolfssl.com/documentation/CyaSSL-Manual.pdf.  For build instructions
and comments about the new features please check the manual.


# CyaSSL Release 3.2.0 (09/10/2014)

#### Release 3.2.0 CyaSSL has bug fixes and new features including:

- ChaCha20 and Poly1305 crypto and suites
- Small stack improvements for OCSP, CRL, TLS, DTLS
- NTRU Encrypt and Decrypt benchmarks
- Updated Visual Studio project files
- Updated Keil MDK5 project files
- Fix for DTLS sequence numbers with GCM/CCM
- Updated HashDRBG with more secure struct declaration
- TI-RTOS support and example Code Composer Studio project files
- Ability to get enabled cipher suites, CyaSSL_get_ciphers()
- AES-GCM/CCM/Direct support for Freescale mmCAU and CAU
- Sniffer improvement checking for decrypt key setup
- Support for raw ECC key import
- Ability to convert ecc_key to DER, EccKeyToDer()
- Security fix for RSA Padding check vulnerability reported by Intel Security
  Advanced Threat Research team

The CyaSSL manual is available at:
http://www.wolfssl.com/documentation/CyaSSL-Manual.pdf.  For build instructions
and comments about the new features please check the manual.


# CyaSSL Release 3.1.0 (07/14/2014)

#### Release 3.1.0 CyaSSL has bug fixes and new features including:

- Fix for older versions of icc without 128-bit type
- Intel ASM syntax for AES-NI
- Updated NTRU support, keygen benchmark
- FIPS check for minimum required HMAC key length
- Small stack (--enable-smallstack) improvements for PKCS#7, ASN
- TLS extension support for DTLS
- Default I/O callbacks external to user
- Updated example client with bad clock test
- Ability to set optional ECC context info
- Ability to enable/disable DH separate from opensslextra
- Additional test key/cert buffers for CA and server
- Updated example certificates

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions
and comments about the new features please check the manual.


# CyaSSL Release 3.0.2 (05/30/2014)

#### Release 3.0.2 CyaSSL has bug fixes and new features including:

- Added the following cipher suites:
  * TLS_PSK_WITH_AES_128_GCM_SHA256
  * TLS_PSK_WITH_AES_256_GCM_SHA384
  * TLS_PSK_WITH_AES_256_CBC_SHA384
  * TLS_PSK_WITH_NULL_SHA384
  * TLS_DHE_PSK_WITH_AES_128_GCM_SHA256
  * TLS_DHE_PSK_WITH_AES_256_GCM_SHA384
  * TLS_DHE_PSK_WITH_AES_128_CBC_SHA256
  * TLS_DHE_PSK_WITH_AES_256_CBC_SHA384
  * TLS_DHE_PSK_WITH_NULL_SHA256
  * TLS_DHE_PSK_WITH_NULL_SHA384
  * TLS_DHE_PSK_WITH_AES_128_CCM
  * TLS_DHE_PSK_WITH_AES_256_CCM
- Added AES-NI support for Microsoft Visual Studio builds.
- Changed small stack build to be disabled by default.
- Updated the Hash DRBG and provided a configure option to enable.

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions
and comments about the new features please check the manual.


# CyaSSL Release 3.0.0 (04/29/2014)

#### Release 3.0.0 CyaSSL has bug fixes and new features including:

- FIPS release candidate
- X.509 improvements that address items reported by Suman Jana with security
  researchers at UT Austin and UC Davis
- Small stack size improvements, --enable-smallstack. Offloads large local
  variables to the heap. (Note this is not complete.)
- Updated AES-CCM-8 cipher suites to use approved suite numbers.

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions
and comments about the new features please check the manual.


# CyaSSL Release 2.9.4 (04/09/2014)

#### Release 2.9.4 CyaSSL has bug fixes and new features including:

- Security fixes that address items reported by Ivan Fratric of the Google
  Security Team
- X.509 Unknown critical extensions treated as errors, report by Suman Jana with
  security researchers at UT Austin and UC Davis
- Sniffer fixes for corrupted packet length and Jumbo frames
- ARM thumb mode assembly fixes
- Xcode 5.1 support including new clang
- PIC32 MZ hardware support
- CyaSSL Object has enough room to read the Record Header now w/o allocs
- FIPS wrappers for AES, 3DES, SHA1, SHA256, SHA384, HMAC, and RSA.
- A sample I/O pool is demonstrated with --enable-iopool to overtake memory
  handling and reduce memory fragmentation on I/O large sizes

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.


# CyaSSL Release 2.9.0 (02/07/2014)

#### Release 2.9.0 CyaSSL has bug fixes and new features including:
- Freescale Kinetis RNGB support
- Freescale Kinetis mmCAU support
- TLS Hello extensions
  - ECC
  - Secure Renegotiation (null) 
  - Truncated HMAC
- SCEP support
  - PKCS #7 Enveloped data and signed data
  - PKCS #10 Certificate Signing Request generation
- DTLS sliding window
- OCSP Improvements
  - API change to integrate into Certificate Manager
  - IPv4/IPv6 agnostic
  - example client/server support for OCSP
  - OCSP nonces are optional
- GMAC hashing
- Windows build additions
- Windows CYGWIN build fixes
- Updated test certificates
- Microchip MPLAB Harmony support
- Update autoconf scripts
- Additional X.509 inspection functions
- ECC encrypt/decrypt primitives
- ECC Certificate generation

The Freescale Kinetis K53 RNGB documentation can be found in Chapter 33 of the
K53 Sub-Family Reference Manual:
http://cache.freescale.com/files/32bit/doc/ref_manual/K53P144M100SF2RM.pdf

Freescale Kinetis K60 mmCAU (AES, DES, 3DES, MD5, SHA, SHA256) documentation
can be found in the "ColdFire/ColdFire+ CAU and Kinetis mmCAU Software Library
User Guide":
http://cache.freescale.com/files/32bit/doc/user_guide/CAUAPIUG.pdf


# CyaSSL Release 2.8.0 (8/30/2013)

#### Release 2.8.0 CyaSSL has bug fixes and new features including:
- AES-GCM and AES-CCM use AES-NI
- NetX default IO callback handlers
- IPv6 fixes for DTLS Hello Cookies
- The ability to unload Certs/Keys after the handshake, CyaSSL_UnloadCertsKeys()
- SEP certificate extensions
- Callback getters for easier resource freeing
- External CYASSL_MAX_ERROR_SZ for correct error buffer sizing
- MacEncrypt and DecryptVerify Callbacks for User Atomic Record Layer Processing
- Public Key Callbacks for ECC and RSA
- Client now sends blank cert upon request if doesn't have one with TLS <= 1.2


The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.


# CyaSSL Release 2.7.0 (6/17/2013)

#### Release 2.7.0 CyaSSL has bug fixes and new features including:
- SNI support for client and server
- KEIL MDK-ARM projects
- Wildcard check to domain name match, and Subject altnames are checked too
- Better error messages for certificate verification errors
- Ability to discard session during handshake verify
- More consistent error returns across all APIs
- Ability to unload CAs at the CTX or CertManager level
- Authority subject id support for Certificate matching
- Persistent session cache functionality
- Persistent CA cache functionality
- Client session table lookups to push serverID table to library level
- Camellia support to sniffer
- User controllable settings for DTLS timeout values
- Sniffer fixes for caching long lived sessions
- DTLS reliability enhancements for the handshake
- Better ThreadX support

When compiling with Mingw, libtool may give the following warning due to
path conversion errors:
 
```
libtool: link: Could not determine host file name corresponding to **
libtool: link: Continuing, but uninstalled executables may not work.
```

If so, examples and testsuite will have problems when run, showing an
error while loading shared libraries. To resolve, please run "make install".

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.


# CyaSSL Release 2.6.0 (04/15/2013)

#### Release 2.6.0 CyaSSL has bug fixes and new features including:
- DTLS 1.2 support including AEAD ciphers
- SHA-3 finalist Blake2 support, it's fast and uses little resources
- SHA-384 cipher suites including ECC ones
- HMAC now supports SHA-512
- Track memory use for example client/server with -t option
- Better IPv6 examples with --enable-ipv6, before if ipv6 examples/tests were
  turned on, localhost only was used.  Now link-local (with scope ids) and ipv6
  hosts can be used as well.
- Xcode v4.6 project for iOS v6.1 update
- settings.h is now checked in all *.c files for true one file setting detection
- Better alignment at SSL layer for hardware crypto alignment needs
    * Note, SSL itself isn't friendly to alignment with 5 byte TLS headers and
      13 bytes DTLS headers, but every effort is now made to align with the
      CYASSL_GENERAL_ALIGNMENT flag which sets desired alignment requirement
- NO_64BIT flag to turn off 64bit data type accumulators in public key code
    * Note, some systems are faster with 32bit accumulators 
- --enable-stacksize for example client/server stack use
    * Note, modern desktop Operating Systems may add bytes to each stack frame
- Updated compression/decompression with direct crypto access
- All ./configure options are now lowercase only for consistency
- ./configure builds default to fastmath option
    * Note, if on ia32 and building in shared mode this may produce a problem
      with a missing register being available because of PIC, there are at least
      6 solutions to this:
      1) --disable-fastmath , don't use fastmath
      2) --disable-shared, don't build a shared library
      3) C_EXTRA_FLAGS=-DTFM_NO_ASM , turn off assembly use
      4) use clang, it just seems to work
      5) play around with no PIC options to force all registers being open,
         e.g., --without-pic
      6) if static lib is still a problem try removing fPIE
- Many new ./configure switches for option enable/disable for example
    * rsa
    * dh
    * dsa
    * md5
    * sha 
    * arc4
    * null    (allow NULL ciphers)
    * oldtls  (only use TLS 1.2)
    * asn     (no certs or public keys allowed)
- ./configure generates cyassl/options.h which allows a header the user can 
  include in their app to make sure the same options are set at the app and
  CyaSSL level.
- autoconf no longer needs serial-tests which lowers version requirements of
  automake to 1.11 and autoconf to 2.63

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.5.0 (02/04/2013)

#### Release 2.5.0 CyaSSL has bug fixes and new features including:
- Fix for TLS CBC padding timing attack identified by Nadhem Alfardan and
  Kenny Paterson: http://www.isg.rhul.ac.uk/tls/
- Microchip PIC32 (MIPS16, MIPS32) support
- Microchip MPLAB X example projects for PIC32 Ethernet Starter Kit
- Updated CTaoCrypt benchmark app for embedded systems
- 1024-bit test certs/keys and cert/key buffers
- AES-CCM-8 crypto and cipher suites
- Camellia crypto and cipher suites
- Bumped minimum autoconf version to 2.65, automake version to 1.12
- Addition of OCSP callbacks
- STM32F2 support with hardware crypto and RNG 
- Cavium NITROX support

CTaoCrypt now has support for the Microchip PIC32 and has been tested with
the Microchip PIC32 Ethernet Starter Kit, the XC32 compiler and
MPLAB X IDE in both MIPS16 and MIPS32 instruction set modes. See the README
located under the <cyassl_root>/mplabx directory for more details.

To add Cavium NITROX support do:

./configure --with-cavium=/home/user/cavium/software

pointing to your licensed cavium/software directory.  Since Cavium doesn't
build a library we pull in the cavium_common.o file which gives a libtool 
warning about the portability of this.  Also, if you're using the github source
tree you'll need to remove the -Wredundant-decls warning from the generated
Makefile because the cavium headers don't conform to this warning.  Currently
CyaSSL supports Cavium RNG, AES, 3DES, RC4, HMAC, and RSA directly at the crypto
layer.  Support at the SSL level is partial and currently just does AES, 3DES,
and RC4.  RSA and HMAC are slower until the Cavium calls can be utilized in non
blocking mode.  The example client turns on cavium support as does the crypto
test and benchmark.  Please see the HAVE_CAVIUM define.

CyaSSL is able to use the STM32F2 hardware-based cryptography and random number
generator through the STM32F2 Standard Peripheral Library. For necessary
defines, see the CYASSL_STM32F2 define in settings.h. Documentation for the
STM32F2 Standard Peripheral Library can be found in the following document: 
http://www.st.com/internet/com/TECHNICAL_RESOURCES/TECHNICAL_LITERATURE/USER_MANUAL/DM00023896.pdf

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.4.6 (12/20/2012)

#### Release 2.4.6 CyaSSL has bug fixes and a few new features including:
- ECC into main version
- Lean PSK build (reduced code size, RAM usage, and stack usage)
- FreeBSD CRL monitor support
- CyaSSL_peek()
- CyaSSL_send() and CyaSSL_recv() for I/O flag setting
- CodeWarrior Support
- MQX Support
- Freescale Kinetis support including Hardware RNG
- autoconf builds use jobserver
- cyassl-config
- Sniffer memory reductions

Thanks to Brian Aker for the improved autoconf system, make rpm, cyassl-config,
warning system, and general good ideas for improving CyaSSL!

The Freescale Kinetis K70 RNGA documentation can be found in Chapter 37 of the
K70 Sub-Family Reference Manual:
http://cache.freescale.com/files/microcontrollers/doc/ref_manual/K70P256M150SF3RM.pdf

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.


# CyaSSL Release 2.4.0 (10/10/2012)

#### Release 2.4.0 CyaSSL has bug fixes and a few new features including:
- DTLS reliability
- Reduced memory usage after handshake
- Updated build process

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.3.0 (8/10/2012)

#### Release 2.3.0 CyaSSL has bug fixes and a few new features including:
- AES-GCM crypto and cipher suites
- make test cipher suite checks
- Subject AltName processing
- Command line support for client/server examples
- Sniffer SessionTicket support
- SHA-384 cipher suites
- Verify cipher suite validity when user overrides
- CRL dir monitoring
- DTLS Cookie support, reliability coming soon

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.2.0 (5/18/2012)

#### Release 2.2.0 CyaSSL has bug fixes and a few new features including:
- Initial CRL support (--enable-crl)
- Initial OCSP support (--enable-ocsp)
- Add static ECDH suites
- SHA-384 support
- ECC client certificate support
- Add medium session cache size (1055 sessions) 
- Updated unit tests
- Protection against mutex reinitialization


The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.0.8 (2/24/2012)

#### Release 2.0.8 CyaSSL has bug fixes and a few new features including:
- A fix for malicious certificates pointed out by Remi Gacogne (thanks)
  resulting in NULL pointer use.
- Respond to renegotiation attempt with no_renegoatation alert
- Add basic path support for load_verify_locations()
- Add set Temp EC-DHE key size
- Extra checks on rsa test when porting into


The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.0.6 (1/27/2012)

#### Release 2.0.6 CyaSSL has bug fixes and a few new features including:
- Fixes for CA basis constraint check
- CTX reference counting
- Initial unit test additions
- Lean and Mean Windows fix
- ECC benchmarking
- SSMTP build support
- Ability to group handshake messages with set_group_messages(ctx/ssl)
- CA cache addition callback
- Export Base64_Encode for general use

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.0.2 (12/05/2011)

#### Release 2.0.2 CyaSSL has bug fixes and a few new features including:
- CTaoCrypt Runtime library detection settings when directly using the crypto
  library
- Default certificate generation now uses SHAwRSA and adds SHA256wRSA generation
- All test certificates now use 2048bit and SHA-1 for better modern browser
  support
- Direct AES block access and AES-CTR (counter) mode
- Microchip pic32 support

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.



# CyaSSL Release 2.0.0rc3 (9/28/2011)

#### Release 2.0.0rc3 for CyaSSL has bug fixes and a few new features including:
- updated autoconf support
- better make install and uninstall  (uses system directories)
- make test / make check
- CyaSSL headers now in <cyassl/*.h>
- CTaocrypt headers now in <cyassl/ctaocrypt/*.h>
- OpenSSL compatibility headers now in <cyassl/openssl/*.h>
- examples and tests all run from home directory so can use certs in ./certs
        (see note 1)

So previous applications that used the OpenSSL compatibility header
<openssl/ssl.h> now need to include <cyassl/openssl/ssl.h> instead, no other
changes are required.

Special Thanks to Brian Aker for his autoconf, install, and header patches.

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.

# CyaSSL Release 2.0.0rc2 (6/6/2011)

#### Release 2.0.0rc2 for CyaSSL has bug fixes and a few new features including:
- bug fixes (Alerts, DTLS with DHE)
- FreeRTOS support
- lwIP support
- Wshadow warnings removed
- asn public header
- CTaoCrypt public headers now all have ctc_ prefix (the manual is still being
        updated to reflect this change)
- and more.

This is the 2nd and perhaps final release candidate for version 2.
Please send any comments or questions to support@yassl.com.

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.

# CyaSSL Release 2.0.0rc1 (5/2/2011)

#### Release 2.0.0rc1 for CyaSSL has many new features including:
- bug fixes
- SHA-256 cipher suites 
- Root Certificate Verification (instead of needing all certs in the chain) 
- PKCS #8 private key encryption (supports PKCS #5 v1-v2 and PKCS #12) 
- Serial number retrieval for x509 
- PBKDF2 and PKCS #12 PBKDF 
- UID parsing for x509 
- SHA-256 certificate signatures 
- Client and server can send chains (SSL_CTX_use_certificate_chain_file) 
- CA loading can now parse multiple certificates per file
- Dynamic memory runtime hooks
- Runtime hooks for logging
- EDH on server side
- More informative error codes
- More informative logging messages
- Version downgrade more robust (use SSL_v23*)
- Shared build only by default through ./configure
- Compiler visibility is now used, internal functions not polluting namespace
- Single Makefile, no recursion, for faster and simpler building
- Turn on all warnings possible build option, warning fixes
- and more.

Because of all the new features and the multiple OS, compiler, feature-set
options that CyaSSL allows, there may be some configuration fixes needed.
Please send any comments or questions to support@yassl.com.

The CyaSSL manual is available at:
http://www.yassl.com/documentation/CyaSSL-Manual.pdf.  For build instructions 
and comments about the new features please check the manual.

# CyaSSL Release 1.9.0 (3/2/2011)

Release 1.9.0 for CyaSSL adds bug fixes, improved TLSv1.2 through testing and
better hash/sig algo ids, --enable-webServer for the yaSSL embedded web server,
improper AES key setup detection, user cert verify callback improvements, and
more.

The CyaSSL manual offering is included in the doc/ directory.  For build
instructions and comments about the new features please check the manual.

Please send any comments or questions to support@yassl.com.

# CyaSSL Release 1.8.0 (12/23/2010)

Release 1.8.0 for CyaSSL adds bug fixes, x509 v3 CA signed certificate
generation, a C standard library abstraction layer, lower memory use, increased
portability through the os_settings.h file, and the ability to use NTRU cipher
suites when used in conjunction with an NTRU license and library.

The initial CyaSSL manual offering is included in the doc/ directory.  For
build instructions and comments about the new features please check the manual.

Please send any comments or questions to support@yassl.com.

Happy Holidays.
 

# CyaSSL Release 1.6.5 (9/9/2010)

Release 1.6.5 for CyaSSL adds bug fixes and x509 v3 self signed certificate
generation.
 
For general build instructions see doc/Building_CyaSSL.pdf.

To enable certificate generation support add this option to ./configure
./configure --enable-certgen

An example is included in ctaocrypt/test/test.c and documentation is provided
in doc/CyaSSL_Extensions_Reference.pdf item 11.

# CyaSSL Release 1.6.0 (8/27/2010)

Release 1.6.0 for CyaSSL adds bug fixes, RIPEMD-160, SHA-512, and RSA key
generation.
 
For general build instructions see doc/Building_CyaSSL.pdf.

To add RIPEMD-160 support add this option to ./configure
./configure --enable-ripemd

To add SHA-512 support add this option to ./configure
./configure --enable-sha512

To add RSA key generation support add this option to ./configure
./configure --enable-keygen

Please see ctaocrypt/test/test.c for examples and usage.

For Windows, RIPEMD-160 and SHA-512 are enabled by default but key generation is
off by default.  To turn key generation on add the define CYASSL_KEY_GEN to
CyaSSL.


# CyaSSL Release 1.5.6 (7/28/2010)

Release 1.5.6 for CyaSSL adds bug fixes, compatibility for our JSSE provider,
and a fix for GCC builds on some systems.
 
For general build instructions see doc/Building_CyaSSL.pdf.

To add AES-NI support add this option to ./configure
./configure --enable-aesni

You'll need GCC 4.4.3 or later to make use of the assembly.

# CyaSSL Release 1.5.4 (7/7/2010)

Release 1.5.4 for CyaSSL adds bug fixes, support for AES-NI, SHA1 speed 
improvements from loop unrolling, and support for the Mongoose Web Server.
 
For general build instructions see doc/Building_CyaSSL.pdf.

To add AES-NI support add this option to ./configure
./configure --enable-aesni

You'll need GCC 4.4.3 or later to make use of the assembly.

# CyaSSL Release 1.5.0 (5/11/2010)

Release 1.5.0 for CyaSSL adds bug fixes, GoAhead WebServer support, sniffer
support, and initial swig interface support.

For general build instructions see doc/Building_CyaSSL.pdf.

To add support for GoAhead WebServer either --enable-opensslExtra or if you
don't want all the features of opensslExtra you can just define GOAHEAD_WS
instead.  GOAHEAD_WS can be added to ./configure with CFLAGS=-DGOAHEAD_WS or
you can define it yourself.

To look at the sniffer support please see the sniffertest app in
sslSniffer/sslSnifferTest.  Build with --enable-sniffer on *nix or use the
vcproj files on windows.  You'll need to have pcap installed on *nix and
WinPcap on windows.

A swig interface file is now located in the swig directory for using Python,
Java, Perl, and others with CyaSSL.  This is initial support and experimental,
please send questions or comments to support@yassl.com.

When doing load testing with CyaSSL, on the echoserver example say, the client
machine may run out of tcp ephemeral ports, they will end up in the TIME_WAIT
queue, and can't be reused by default.  There are generally two ways to fix
this. 

1. Reduce the length sockets remain on the TIME_WAIT queue OR
2. Allow items on the TIME_WAIT queue to be reused.


To reduce the TIME_WAIT length in OS X to 3 seconds (3000 milliseconds)

`sudo sysctl -w net.inet.tcp.msl=3000`

In Linux

`sudo sysctl -w net.ipv4.tcp_tw_reuse=1`

allows reuse of sockets in TIME_WAIT

`sudo sysctl -w net.ipv4.tcp_tw_recycle=1`

works but seems to remove sockets from  TIME_WAIT entirely?

`sudo sysctl -w net.ipv4.tcp_fin_timeout=1`

doen't control TIME_WAIT, it controls FIN_WAIT(2) contrary to some posts


# CyaSSL Release 1.4.0 (2/18/2010)

Release 1.3.0 for CyaSSL adds bug fixes, better multi TLS/SSL version support
through SSLv23_server_method(), and improved documentation in the doc/ folder.

For general build instructions doc/Building_CyaSSL.pdf.

# CyaSSL Release 1.3.0 (1/21/2010)

Release 1.3.0 for CyaSSL adds bug fixes, a potential security problem fix,
better porting support, removal of assert()s, and a complete THREADX port.

For general build instructions see rc1 below.

# CyaSSL Release 1.2.0 (11/2/2009)

Release 1.2.0 for CyaSSL adds bug fixes and session negotiation if first use is
read or write.

For general build instructions see rc1 below.

# CyaSSL Release 1.1.0 (9/2/2009)

Release 1.1.0 for CyaSSL adds bug fixes, a check against malicious session
cache use, support for lighttpd, and TLS 1.2.

To get TLS 1.2 support please use the client and server functions:

```c
SSL_METHOD *TLSv1_2_server_method(void);
SSL_METHOD *TLSv1_2_client_method(void);
```

CyaSSL was tested against lighttpd 1.4.23.  To build CyaSSL for use with 
lighttpd use the following commands from the CyaSSL install dir <CyaSSLDir>:

```
./configure --disable-shared --enable-opensslExtra --enable-fastmath --without-zlib

make
make openssl-links
```

Then to build lighttpd with CyaSSL use the following commands from the
lighttpd install dir:

```
./configure --with-openssl --with-openssl-includes=<CyaSSLDir>/include --with-openssl-libs=<CyaSSLDir>/lib LDFLAGS=-lm

make
```

On some systems you may get a linker error about a duplicate symbol for
MD5_Init or other MD5 calls.  This seems to be caused by the lighttpd src file
md5.c, which defines MD5_Init(), and is included in liblightcomp_la-md5.o.
When liblightcomp is linked with the SSL_LIBs the linker may complain about
the duplicate symbol.  This can be fixed by editing the lighttpd src file md5.c
and adding this line to the beginning of the file:

\#if 0

and this line to the end of the file

\#endif

Then from the lighttpd src dir do a:

```
make clean
make
```

If you get link errors about undefined symbols more than likely the actual
OpenSSL libraries are found by the linker before the CyaSSL openssl-links that
point to the CyaSSL library, causing the linker confusion.  This can be fixed
by editing the Makefile in the lighttpd src directory and changing the line:

`SSL_LIB = -lssl -lcrypto`

to

`SSL_LIB = -lcyassl`

Then from the lighttpd src dir do a:

```
make clean
make
```

This should remove any confusion the linker may be having with missing symbols.

For any questions or concerns please contact support@yassl.com .

For general build instructions see rc1 below.

# CyaSSL Release 1.0.6 (8/03/2009)

Release 1.0.6 for CyaSSL adds bug fixes, an improved session cache, and faster
math with a huge code option.

The session cache now defaults to a client mode, also good for embedded servers.
For servers not under heavy load (less than 200 new sessions per minute), define
BIG_SESSION_CACHE.  If the server will be under heavy load, define
HUGE_SESSION_CACHE.

There is now a fasthugemath option for configure.  This enables fastmath plus
even faster math by greatly increasing the code size of the math library. Use
the benchmark utility to compare public key operations.


For general build instructions see rc1 below.

# CyaSSL Release 1.0.3 (5/10/2009)

Release 1.0.3 for CyaSSL adds bug fixes and add increased support for OpenSSL
compatibility when building other applications.

Release 1.0.3 includes an alpha release of DTLS for both client and servers.
This is only for testing purposes at this time.  Rebroadcast and reordering
aren't fully implemented at this time but will be for the next release.

For general build instructions see rc1 below.

# CyaSSL Release 1.0.2 (4/3/2009)

Release 1.0.2 for CyaSSL adds bug fixes for a couple I/O issues.  Some systems
will send a SIGPIPE on socket recv() at any time and this should be handled by
the application by turning off SIGPIPE through setsockopt() or returning from
the handler.

Release 1.0.2 includes an alpha release of DTLS for both client and servers.
This is only for testing purposes at this time.  Rebroadcast and reordering
aren't fully implemented at this time but will be for the next release.

For general build instructions see rc1 below.

## CyaSSL Release Candidate 3 rc3-1.0.0 (2/25/2009)


Release Candidate 3 for CyaSSL 1.0.0 adds bug fixes and adds a project file for
iPhone development with Xcode.  cyassl-iphone.xcodeproj is located in the root
directory.  This release also includes a fix for supporting other
implementations that bundle multiple messages at the record layer, this was
lost when cyassl i/o was re-implemented but is now fixed.

For general build instructions see rc1 below.

## CyaSSL Release Candidate 2 rc2-1.0.0 (1/21/2009)


Release Candidate 2 for CyaSSL 1.0.0 adds bug fixes and adds two new stream
ciphers along with their respective cipher suites.  CyaSSL adds support for
HC-128 and RABBIT stream ciphers.  The new suites are:

```
TLS_RSA_WITH_HC_128_SHA
TLS_RSA_WITH_RABBIT_SHA
```

And the corresponding cipher names are

```
HC128-SHA
RABBIT-SHA
```

CyaSSL also adds support for building with devkitPro for PPC by changing the
library proper to use libogc.  The examples haven't been changed yet but if
there's interest they can be.  Here's an example ./configure to build CyaSSL
for devkitPro:

```
./configure --disable-shared CC=/pathTo/devkitpro/devkitPPC/bin/powerpc-gekko-gcc --host=ppc --without-zlib --enable-singleThreaded RANLIB=/pathTo/devkitpro/devkitPPC/bin/powerpc-gekko-ranlib CFLAGS="-DDEVKITPRO -DGEKKO"
```

For linking purposes you'll need

`LDFLAGS="-g -mrvl -mcpu=750 -meabi -mhard-float -Wl,-Map,$(notdir $@).map"`

For general build instructions see rc1 below.


## CyaSSL Release Candidate 1 rc1-1.0.0 (12/17/2008)


Release Candidate 1 for CyaSSL 1.0.0 contains major internal changes.  Several
areas have optimization improvements, less dynamic memory use, and the I/O
strategy has been refactored to allow alternate I/O handling or Library use.
Many thanks to Thierry Fournier for providing these ideas and most of the work.

Because of these changes, this release is only a candidate since some problems
are probably inevitable on some platform with some I/O use.  Please report any
problems and we'll try to resolve them as soon as possible.  You can contact us
at support@yassl.com or todd@yassl.com.

Using TomsFastMath by passing --enable-fastmath to ./configure now uses assembly
on some platforms.  This is new so please report any problems as every compiler,
mode, OS combination hasn't been tested.  On ia32 all of the registers need to
be available so be sure to pass these options to CFLAGS:

`CFLAGS="-O3 -fomit-frame-pointer"`

OS X will also need -mdynamic-no-pic added to CFLAGS

Also if you're building in shared mode for ia32 you'll need to pass options to
LDFLAGS as well on OS X:

`LDFLAGS=-Wl,-read_only_relocs,warning`

This gives warnings for some symbols but seems to work.


#### To build on Linux, Solaris, *BSD, Mac OS X, or Cygwin:

    ./configure
    make

    from the ./testsuite/ directory run ./testsuite 

#### To make a debug build:

    ./configure --enable-debug --disable-shared
    make



#### To build on Win32

Choose (Re)Build All from the project workspace

Run the testsuite program





# CyaSSL version 0.9.9 (7/25/2008) 

This release of CyaSSL adds bug fixes, Pre-Shared Keys, over-rideable memory
handling, and optionally TomsFastMath.  Thanks to Moisés Guimarães for the
work on TomsFastMath.

To optionally use TomsFastMath pass --enable-fastmath to ./configure
Or define USE_FAST_MATH in each project from CyaSSL for MSVC.

Please use the benchmark routine before and after to see the performance
difference, on some platforms the gains will be little but RSA encryption
always seems to be faster.  On x86-64 machines with GCC the normal math library
may outperform the fast one when using CFLAGS=-m64 because TomsFastMath can't
yet use -m64 because of GCCs inability to do 128bit division.

     *** UPDATE GCC 4.2.1 can now do 128bit division ***

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.9.8 (5/7/2008) 

This release of CyaSSL adds bug fixes, client side Diffie-Hellman, and better
socket handling.

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.9.6 (1/31/2008) 

This release of CyaSSL adds bug fixes, increased session management, and a fix
for gnutls.

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.9.0 (10/15/2007) 

This release of CyaSSL adds bug fixes, MSVC 2005 support, GCC 4.2 support, 
IPV6 support and test, and new test certificates.

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.8.0 (1/10/2007) 

This release of CyaSSL adds increased socket support, for non-blocking writes,
connects, and interrupted system calls.

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.6.3 (10/30/2006) 

This release of CyaSSL adds debug logging to stderr to aid in the debugging of
CyaSSL on systems that may not provide the best support.

If CyaSSL is built with debugging support then you need to call
CyaSSL_Debugging_ON() to turn logging on.

On Unix use ./configure --enable-debug

On Windows define DEBUG_CYASSL when building CyaSSL


To turn logging back off call CyaSSL_Debugging_OFF()

See notes below (0.2.0) for complete build instructions.


# CyaSSL version 0.6.2 (10/29/2006) 

This release of CyaSSL adds TLS 1.1.

Note that CyaSSL has certificate verification on by default, unlike OpenSSL.
To emulate OpenSSL behavior, you must call SSL_CTX_set_verify() with
SSL_VERIFY_NONE.  In order to have full security you should never do this, 
provide CyaSSL with the proper certificates to eliminate impostors and call
CyaSSL_check_domain_name() to prevent man in the middle attacks.

See notes below (0.2.0) for build instructions.

# CyaSSL version 0.6.0 (10/25/2006) 

This release of CyaSSL adds more SSL functions, better autoconf, nonblocking
I/O for accept, connect, and read.  There is now an --enable-small configure
option that turns off TLS, AES, DES3, HMAC, and ERROR_STRINGS, see configure.in
for the defines.  Note that TLS requires HMAC and AES requires TLS.

See notes below (0.2.0) for build instructions.


# CyaSSL version 0.5.5 (09/27/2006) 

This mini release of CyaSSL adds better input processing through buffered input
and big message support.  Added SSL_pending() and some sanity checks on user
settings.

See notes below (0.2.0) for build instructions.


# CyaSSL version 0.5.0 (03/27/2006) 

This release of CyaSSL adds AES support and minor bug fixes. 

See notes below (0.2.0) for build instructions.


# CyaSSL version 0.4.0 (03/15/2006)

This release of CyaSSL adds TLSv1 client/server support and libtool. 

See notes below for build instructions.


# CyaSSL version 0.3.0 (02/26/2006)

This release of CyaSSL adds SSLv3 server support and session resumption. 

See notes below for build instructions.


# CyaSSL version 0.2.0 (02/19/2006)


This is the first release of CyaSSL and its crypt brother, CTaoCrypt.  CyaSSL
is written in ANSI C with the idea of a small code size, footprint, and memory
usage in mind.  CTaoCrypt can be as small as 32K, and the current client
version of CyaSSL can be as small as 12K.


The first release of CTaoCrypt supports MD5, SHA-1, 3DES, ARC4, Big Integer
Support, RSA, ASN parsing, and basic x509 (en/de)coding.

The first release of CyaSSL supports normal client RSA mode SSLv3 connections
with support for SHA-1 and MD5 digests.  Ciphers include 3DES and RC4.


#### To build on Linux, Solaris, *BSD, Mac OS X, or Cygwin:

    ./configure
    make

    from the ./testsuite/ directory run ./testsuite 

#### to make a debug build:

    ./configure --enable-debug --disable-shared
    make



#### To build on Win32

Choose (Re)Build All from the project workspace

Run the testsuite program



*** The next release of CyaSSL will support a server and more OpenSSL
compatibility functions.


Please send questions or comments to todd@wolfssl.com
