# wolfSSL JNI/JSSE and wolfCrypt JNI/JCE Android Examples

This directory contains two Android Studio projects that demonstrate how
wolfCrypt JNI/JCE and wolfSSL JNI/JSSE can be integrated into an
Android Studio project using the Gradle build system to compile both
shared libraries and the application.

[wolfcryptjni-ndk-gradle](./wolfcryptjni-ndk-gradle)

[wolfssljni-ndk-gradle](./wolfssljni-ndk-gradle)

This directory also contains one older example of how wolfSSL JNI/JSSE
could be integrated into an older standalone NDK toolchain project's Android.mk
build file. The above Android Studio projects are the recommended approach
unless using an older standalone NDK toolchain setup.

[wolfssljni-ndk-sample](./wolfssljni-ndk-sample)

wolfSSL also maintains an Android Studio example project that is shipped
as part of the wolfSSL JNI/JSSE bundle. This example project is set up to
run the wolfSSL JNI/JSSE tests on an emulator as well, and is located here:

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

2) Checkout wolfssl and wolfssljni git submodules

3) Create stub options.h (since we're using the GitHub repo for wolfSSL)

4) Open "wolfssljni-ndk-gradle" project in Android Studio and build project

```
$ cd android/wolfssljni-ndk-gradle
$ git submodule init
$ git submodule update
$ cp ./wolfssl/wolfssl/options.h.in ./wolfssl/wolfssl/options.h
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

Or, to use a specific tag or commit, use `git checkout`:

```
$ cd android/wolfssljni-ndk-gradle/wolfssl
$ git checkout vX.X.X-stable
```

To install and run the application in an Android emulator, set one up
in Android Studio, then click the Debug application button on the Android
Studio toolbar. This will allow you to choose what Android virtual machine
you would like to use.

Logcat output from the emulator can be viewed by opening a new terminal window
while the emulator is running and issuing:

```
$ adb logcat
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

