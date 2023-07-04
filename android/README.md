# wolfSSL JNI/JSSE and wolfCrypt JNI/JCE Android Examples

This directory contains two Android Studio projects that demonstrate how
wolfCrypt JNI/JCE and wolfSSL JNI/JSSE can be integrated into an
Android Studio project using the Gradle build system to compile both
shared libraries and the application.

1) [wolfcryptjni-ndk-gradle](./wolfcryptjni-ndk-gradle)

2) [wolfssljni-ndk-gradle](./wolfssljni-ndk-gradle)

This directory also contains one older example of how wolfSSL JNI/JSSE
could be integrated into an older standalone NDK toolchain project's Android.mk
build file. The above Android Studio projects are the recommended approach
unless using an older standalone NDK toolchain setup.

3) [wolfssljni-ndk-sample](./wolfssljni-ndk-sample)

wolfSSL also maintains an Android Studio example project that is shipped
as part of the wolfSSL JNI/JSSE bundle. This example project is set up to
run the wolfSSL JNI/JSSE tests on an emulator and is located here:

[wolfSSL JNI/JSSE IDE/Android Example](https://github.com/wolfSSL/wolfssljni/tree/master/IDE/Android)

## wolfSSL Android Studio NDK Gradle Examples

### Prerequisites for successful installation

In order to build the wolfSSL JNI/JSSE or wolfCrypt JNI/JCE Android Studio
sample applications you need to Install **Android Studio**,
along with **Android NDK** support.

### Compiling and Running wolfssljni-ndk-gradle or wolfcryptjni-ndk-gradle

After a development environment has been set up, follow these instructions
to compile and install either the wolfSSL JNI/JSSE + wolfSSL bundle, or the
wolfCrypt JNI/JCE + wolfSSL bundle on the Emulator:

1) Change directories into the android/wolfssljni-ndk-gradle directory

```
$ cd android/wolfssljni-ndk-gradle
```

2) Checkout wolfssl and wolfssljni git submodules

```
$ git submodule init
$ git submodule update
```

3) Create stub options.h (when using GitHub repo for wolfSSL)

```
$ cp ./wolfssl/wolfssl/options.h.in ./wolfssl/wolfssl/options.h
```

4) Open "wolfssljni-ndk-gradle" project in Android Studio and build project

wolfSSL stable releases, available from the wolfSSL download page, contain
`<wolfssl/options.h>`, but wolfSSL cloned from GitHub does not.
This is why `options.h.in` needs to be copied to `options.h` above for a GitHub
cloned repository.

If you would like to update the git submodules for wolfssl and wolfssljni to
the most current development HEAD, use the following submodule update command
in place of the one above:

```
$ git submodule update --remote
```

Or, to use a specific tag or commit, use `git checkout`:

```
$ cd android/wolfssljni-ndk-gradle/wolfssl
$ git checkout vX.X.X-stable
```

To install and run the application in an Android emulator, set one up
in the Android Studio Device Manager, then click the Debug application button
on the Android Studio toolbar. This will allow you to choose what Android
virtual machine you would like to use.

Logcat output from the emulator can be viewed by opening a new terminal window
while the emulator is running and issuing:

```
$ adb logcat
```

### wolfSSL JNI/JSSE Sample App with wolfSSL FIPS Ready Package

The wolfSSL JNI/JSSE Android Studio gradle sample application can be used
along with the wolfSSL FIPS Ready package, in place of a normal/standard
wolfSSL distribution. To use the `wolfssljni-ndk-gradle` sample app with
wolfSSL FIPS Ready use the following steps.

For more information about wolfSSL FIPS Ready, see the
[wolfSSL Website](https://www.wolfssl.com/license/fips/).

1) Download a GPLv3-licensed wolfSSL FIPS Ready package from the
   [wolfSSL Download Page](https://www.wolfssl.com/download/).

2) Move the archive under the `wolfssljni-ndk-gradle` app directory, extract,
   and rename to `wolfssl`:

```
$ cd wolfssl-examples/android/wolfssljni-ndk-gradle
$ mv /path/to/wolfssl-X.X.X-gplv3-fips-ready.zip ./
$ unzip wolfssl-X.X.X-gplv3-fips-ready.zip
$ mv wolfssl-X.X.X-gplv3-fips-ready wolfssl
```

3) Edit the following `CMakeLists.txt` file and change `WOLFSSL_PKG_TYPE` to
   `fipsready`:

```
wolfssl-examples/android/wolfssljni-ndk-gradle/app/CMakeLists.txt
```

The `WOLFSSL_PKG_TYPE` selection should look like:

```
set(WOLFSSL_PKG_TYPE "fipsready")
```

4) Compile and Install the application on a device or in an emulator

5) Run the "WOLFCRYPT TEST" selection to run the wolfCrypt test application.
You will need to monitor the adb logcat output to see the expected
in-core integrity hash. wolfSSL FIPS Ready does an in-core integrity check
using HMAC-SHA256 over the object files within the FIPS Ready boundary. The
calculated HMAC at runtime is compared to an expected HMAC. If the values do
not match, access is not allowed into the wolfCrypt library.

Upon first compilation, or if settings change and the library is recompiled,
the expected verifyCore[] needs to be updated. To do this:

a) Copy the expected hash from the "adb logcat" output:

```
D/[WOLFCRYPT]: in my Fips callback, ok = 0, err = -203
D/[WOLFCRYPT]: message = In Core Integrity check FIPS error
D/[WOLFCRYPT]: hash = C4EAF104446F5B6918B266A5D65223C8E7ADED2CC41D547ED0C855A50858DE82
D/[WOLFCRYPT]: In core integrity hash check failure, copy above hash
D/[WOLFCRYPT]: into verifyCore[] in fips_test.c and rebuild
```

b) Open `app/wolfssl/wolfcrypt/src/fips_test.c` and copy the hash into the
   `verifyCore[]` array and save the file.

c) Re-compile the application again, and re-run the wolfCrypt test.

### Sample Application Functionality

#### Native wolfCrypt Test Application

The sample application has a button that runs the native wolfCrypt tests. This
wraps and runs the wolfCrypt test from wolfSSL (./wolfcrypt/test/test.c). Ouput
will be printed to the logcat log.

After clicking this button, wolfCrypt tests for all enabled algorithms will
run and print the test status to the logcat log:

```
[WOLFCRYPT]: ------------------------------------------------------------------------------
[WOLFCRYPT]:  wolfSSL version 5.3.0
[WOLFCRYPT]: ------------------------------------------------------------------------------
[WOLFCRYPT]: error    test passed!
[WOLFCRYPT]: MEMORY   test passed!
[WOLFCRYPT]: base64   test passed!
[WOLFCRYPT]: base16   test passed!
[WOLFCRYPT]: asn      test passed!
[WOLFCRYPT]: RANDOM   test passed!
[WOLFCRYPT]: MD5      test passed!
[WOLFCRYPT]: SHA      test passed!
[WOLFCRYPT]: SHA-224  test passed!
[WOLFCRYPT]: SHA-256  test passed!
[WOLFCRYPT]: SHA-384  test passed!
[WOLFCRYPT]: SHA-512  test passed!
[WOLFCRYPT]: SHA-3    test passed!
[WOLFCRYPT]: Hash     test passed!
[WOLFCRYPT]: HMAC-MD5 test passed!
[WOLFCRYPT]: HMAC-SHA test passed!
[WOLFCRYPT]: HMAC-SHA224 test passed!
[WOLFCRYPT]: HMAC-SHA256 test passed!
[WOLFCRYPT]: HMAC-SHA384 test passed!
[WOLFCRYPT]: HMAC-SHA512 test passed!
[WOLFCRYPT]: HMAC-SHA3   test passed!
[WOLFCRYPT]: HMAC-KDF    test passed!
[WOLFCRYPT]: TLSv1.3 KDF test passed!
[WOLFCRYPT]: GMAC     test passed!
[WOLFCRYPT]: Chacha   test passed!
[WOLFCRYPT]: POLY1305 test passed!
[WOLFCRYPT]: ChaCha20-Poly1305 AEAD test passed!
[WOLFCRYPT]: AES      test passed!
[WOLFCRYPT]: AES192   test passed!
[WOLFCRYPT]: AES256   test passed!
[WOLFCRYPT]: AES-GCM  test passed!
[WOLFCRYPT]: RSA NOPAD test passed!
[WOLFCRYPT]: RSA      test passed!
[WOLFCRYPT]: DH       test passed!
[WOLFCRYPT]: PWDBASED test passed!
[WOLFCRYPT]: OPENSSL  test passed!
[WOLFCRYPT]: OPENSSL (EVP MD) passed!
[WOLFCRYPT]: OPENSSL (PKEY0) passed!
[WOLFCRYPT]: OPENSSL (PKEY1) passed!
[WOLFCRYPT]: OPENSSL (EVP Sign/Verify) passed!
[WOLFCRYPT]: ECC      test passed!
[WOLFCRYPT]: ECC buffer test passed!
[WOLFCRYPT]: logging  test passed!
[WOLFCRYPT]: time test passed!
[WOLFCRYPT]: mutex    test passed!
[WOLFCRYPT]: memcb    test passed!
[WOLFCRYPT]: Test complete
```

#### Native wolfCrypt Benchmark Application

The sample application has a button that runs the native wolfCrypt benchmarks.
This wraps the wolfCrypt benchmark from wolfSSL
(./wolfcrypt/benchmark/benchmark.c). Output will be printed to the logcat log.

After clicking on this button, wolfCrypt benchmarks will be run for all
enabled algorithms and be printed to the logcat log.

The following is example output when run on a Pixel 5 API 31
(Android 12, Google APIs) x86\_64 emulator with SP math enabled:

```
[WOLFCRYPT]: wolfCrypt Benchmark (block bytes 1048576, min 1.0 sec each)
[WOLFCRYPT]: RNG                 15 MB took 1.149 seconds,   13.052 MB/s Cycles per byte = 218.79
[WOLFCRYPT]: AES-128-CBC-enc     40 MB took 1.009 seconds,   39.660 MB/s Cycles per byte =  72.00
[WOLFCRYPT]: AES-128-CBC-dec     40 MB took 1.056 seconds,   37.864 MB/s Cycles per byte =  75.42
[WOLFCRYPT]: AES-192-CBC-enc     40 MB took 1.032 seconds,   38.777 MB/s Cycles per byte =  73.64
[WOLFCRYPT]: AES-192-CBC-dec     40 MB took 1.076 seconds,   37.162 MB/s Cycles per byte =  76.84
[WOLFCRYPT]: AES-256-CBC-enc     40 MB took 1.092 seconds,   36.617 MB/s Cycles per byte =  77.99
[WOLFCRYPT]: AES-256-CBC-dec     40 MB took 1.121 seconds,   35.697 MB/s Cycles per byte =  80.00
[WOLFCRYPT]: AES-128-GCM-enc     30 MB took 1.172 seconds,   25.607 MB/s Cycles per byte = 111.52
[WOLFCRYPT]: AES-128-GCM-dec     30 MB took 1.169 seconds,   25.656 MB/s Cycles per byte = 111.31
[WOLFCRYPT]: AES-192-GCM-enc     25 MB took 1.214 seconds,   20.589 MB/s Cycles per byte = 138.70
[WOLFCRYPT]: AES-192-GCM-dec     25 MB took 1.008 seconds,   24.803 MB/s Cycles per byte = 115.13
[WOLFCRYPT]: AES-256-GCM-enc     25 MB took 1.059 seconds,   23.601 MB/s Cycles per byte = 121.00
[WOLFCRYPT]: AES-256-GCM-dec     25 MB took 1.045 seconds,   23.935 MB/s Cycles per byte = 119.31
[WOLFCRYPT]: GMAC Table 4-bit    69 MB took 1.000 seconds,   68.992 MB/s Cycles per byte =  41.39
[WOLFCRYPT]: CHACHA              35 MB took 1.031 seconds,   33.954 MB/s Cycles per byte =  84.10
[WOLFCRYPT]: CHA-POLY            35 MB took 1.068 seconds,   32.778 MB/s Cycles per byte =  87.12
[WOLFCRYPT]: MD5                135 MB took 1.018 seconds,  132.625 MB/s Cycles per byte =  21.53
[WOLFCRYPT]: POLY1305           395 MB took 1.001 seconds,  394.472 MB/s Cycles per byte =   7.24
[WOLFCRYPT]: SHA                 75 MB took 1.065 seconds,   70.436 MB/s Cycles per byte =  40.54
[WOLFCRYPT]: SHA-224             30 MB took 1.010 seconds,   29.705 MB/s Cycles per byte =  96.13
[WOLFCRYPT]: SHA-256             35 MB took 1.162 seconds,   30.129 MB/s Cycles per byte =  94.78
[WOLFCRYPT]: SHA-384             45 MB took 1.051 seconds,   42.811 MB/s Cycles per byte =  66.70
[WOLFCRYPT]: SHA-512             45 MB took 1.053 seconds,   42.745 MB/s Cycles per byte =  66.81
[WOLFCRYPT]: SHA3-224            85 MB took 1.063 seconds,   80.000 MB/s Cycles per byte =  35.70
[WOLFCRYPT]: SHA3-256            80 MB took 1.046 seconds,   76.462 MB/s Cycles per byte =  37.35
[WOLFCRYPT]: SHA3-384            60 MB took 1.002 seconds,   59.866 MB/s Cycles per byte =  47.70
[WOLFCRYPT]: SHA3-512            45 MB took 1.074 seconds,   41.909 MB/s Cycles per byte =  68.14
[WOLFCRYPT]: HMAC-MD5           135 MB took 1.012 seconds,  133.384 MB/s Cycles per byte =  21.41
[WOLFCRYPT]: HMAC-SHA            75 MB took 1.058 seconds,   70.917 MB/s Cycles per byte =  40.27
[WOLFCRYPT]: HMAC-SHA224         35 MB took 1.136 seconds,   30.822 MB/s Cycles per byte =  92.65
[WOLFCRYPT]: HMAC-SHA256         30 MB took 1.004 seconds,   29.888 MB/s Cycles per byte =  95.54
[WOLFCRYPT]: HMAC-SHA384         45 MB took 1.062 seconds,   42.370 MB/s Cycles per byte =  67.40
[WOLFCRYPT]: HMAC-SHA512         45 MB took 1.069 seconds,   42.093 MB/s Cycles per byte =  67.84
[WOLFCRYPT]: PBKDF2               4 KB took 1.008 seconds,    3.598 KB/s Cycles per byte = 812776.94
[WOLFCRYPT]: RSA     2048 public      18700 ops took 1.000 sec, avg 0.053 ms, 18693.849 ops/sec
[WOLFCRYPT]: RSA     2048 private       900 ops took 1.001 sec, avg 1.112 ms, 899.441 ops/sec
[WOLFCRYPT]: DH      2048 key gen      1761 ops took 1.000 sec, avg 0.568 ms, 1760.757 ops/sec
[WOLFCRYPT]: DH      2048 agree        1800 ops took 1.004 sec, avg 0.558 ms, 1792.190 ops/sec
[WOLFCRYPT]: ECC   [      SECP256R1]   256 key gen     33100 ops took 1.003 sec, avg 0.030 ms, 33004.652 ops/sec
[WOLFCRYPT]: ECDHE [      SECP256R1]   256 agree       12200 ops took 1.001 sec, avg 0.082 ms, 12185.694 ops/sec
[WOLFCRYPT]: ECDSA [      SECP256R1]   256 sign        21400 ops took 1.001 sec, avg 0.047 ms, 21373.222 ops/sec
[WOLFCRYPT]: ECDSA [      SECP256R1]   256 verify      11300 ops took 1.006 sec, avg 0.089 ms, 11237.251 ops/sec
[WOLFCRYPT]: Benchmark complete
```

This can be a good tool when optimizing wolfSSL and wolfCrypt for performance.

#### Simple SSLSocket Connection

The sample application has a button that makes a simple SSL/TLS connection
to `wolfssl.com:443` using the SSLSocket class. It does not explicitly load
CA certificates from a KeyStore, since wolfJSSE automatically loads the
Android KeyStore root certificates as trusted, and those are able to
authenticate wolfssl.com.

After clicking on this button, wolfJSSE debug output will be printed to the
logcat log.

#### How to Change wolfSSL Library Configuration

This sample Android application builds native wolfSSL and wolfCrypt sources
into a shared library. That build is done with cmake and controlled by the
CMakeLists.txt file located at:

android/wolfssljni-ndk-gradle/app/CMakeLists.txt

wolfSSL CFLAGS are defined using the `add_definition()` function, and have
been pre-populated in this sample to match those defined when building wolfSSL
with `./configure --enable-jni`.

This CMakeLists.txt has been designed to allow easy switching between wolfSSL's
fastmath math library and the newer SP math library. SP math can offer
performance advantages over the fastmath library, especially when assembly
optimizations are available for a given platform.

To control the math library used, change the variable `WOLFSSL_MATH_LIB`
in CMakeLists.txt:

```
# Math library selection, used to switch on below. Should be one of:
#     fastmath
#     spmath
set(WOLFSSL_MATH_LIB "spmath")
```

## wolfSSL NDK Standalone Toolchain Example

### Prerequisites for successful installation

In order to build the wolfSSL JNI sample application, you need to:

* Install the `Android SDK` [link](http://developer.android.com/sdk/index.html)
* Install the `Android NDK` [link](https://developer.android.com/tools/sdk/ndk/index.html)

Note that these instructions do not use the Android Studio IDE and environment.
This package has been developed and tested using the Android NDK and SDK
command line tools only.

After installing the Android SDK, run the `SDK Tools` app packaged with it and
install Android 4.4:

```
$ android
```

To set up an emulator and create an "Android Virtual Device (avd)" image, run:

```
$ android avd
```

### Compiling and Running the wolfssljni-ndk-sample

After the development environment has been set up, follow these instructions
to compile and install the wolfCrypt JNI + wolfSSL bundle on
the Emulator:

1) Change directories into the android/wolfssljni-ndk-sample directory

2) Checkout wolfssl and wolfssljni git submodules

3) Create stub options.h (since we're using the GitHub repo for wolfSSL)

4) Update Android project as shown below

5) Proceed to compile NDK and Java code as shown below

```
$ cd android/wolfssljni-ndk-sample
$ git submodule init
$ git submodule update
$ cp ./wolfssl/wolfssl/options.h.in ./wolfssl/wolfssl/options.h
$ android update project -p . -s
$ ndk-build
$ ant debug
```

wolfSSL stable releases, available from the wolfSSL download page, contain
<wolfssl/options.h>, but the GitHub development branch for wolfSSL does not.
This is why options.h.in needs to be copied to options.h above.

If you would like to update the git submodules for wolfssl and wolfssljni to
the most current development HEAD, use the following submodule update command
in place of the one above:

```
$ git submodule update --remote
```

To install and run the application in an Android emulator, start the emulator:

```
$ emulator -avd <target_name>
```

Where you can get a list of targets (including the one you created above),
using:

```
$ emulator -list-avds
```

then issue 'ant debug install' from the wolfssljni-ndk-sample directory to
install the apk into the emulator:

```
$ ant debug install
```

Logcat output from the emulator can be viewed by opening a new terminal window
while the emulator is running and issuing:

```
$ adb logcat
```

#### Installing and Running wolfssljni-ndk-sample on Device instead of Emulator

Compiling, installing, and running the wolfssljni-ndk-sample application on a
real device, versus the Android Emulator is quite easy, and nearly identical
to running on the Emulator.

To compile the application, follow the same steps as shown above.

After compilation:

1) Turn on the Android Device

2) Go to "Settings -> Developer options" and verify that "USB Debugging"
   is enabled (checked).

3) Plug Android phone into development computer with USB cable

4) Once the phone has been plugged into the development machine, the same
   command line tools can be used to install apps, view the logcat, and
   pull/push data from/to the device using "adb pull" and "adb push"
   commands. This means you can follow the same steps to install the app
   on the phone, as described above for the emulator - using
   "adb debug install".

## Support

Please contact wolfSSL at support@wolfssl.com with any questions, bug fixes,
or suggested feature additions.

