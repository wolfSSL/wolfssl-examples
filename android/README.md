# wolfSSL / wolfSSL JNI Android Examples

This directory contains:

wolfssljni-ndk-sample - Example project that demonstrates how to integrate
wolfSSL and wolfSSL JNI into an Android NDK application's Android.mk build
file.

## Prerequisites for successful installation

Prerequisites
In order to build the wolfSSL JNI sample application,
you need to:
* Install the `Android SDK` [link](http://developer.android.com/sdk/index.html)
* Install the `Android NDK` [link](https://developer.android.com/tools/sdk/ndk/index.html)

Note that these instructions do not use the Android Studio IDE and environment.
This package has been developed and tested using the Android NDK and SDK
command line tools.

After installing the Android SDK, run the `SDK Tools` app packaged with it and
install Android 4.4:

```
$ android
```

To set up an emulator and create an "Android Virtual Device (avd)" image, run:

```
$ android avd
```

## Compiling and Running the wolfssljni-ndk-sample

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

### Installing and Running wolfssljni-ndk-sample on Device instead of Emulator

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

