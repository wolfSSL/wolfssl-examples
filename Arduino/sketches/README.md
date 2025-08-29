# wolfSSL Arduino Examples

There are currently five example Arduino sketches:

* [template](./template/README.md): Reference template wolfSSL example, including optional VisualGDB project files.
* [wolfssl_AES_CTR](./wolfssl_AES_CTR/README.md): Basic AES CTR Encryption / Decryption example.
* [wolfssl_client](./wolfssl_client/README.md): Basic TLS listening client.
* [wolfssl_server](./wolfssl_server/README.md): Basic TLS server.
* [wolfssl_client_dtls](./wolfssl_client_dtls/README.md): Basic DTLS listening client.
* [wolfssl_server_dtls](./wolfssl_server_dtls/README.md): Basic DTLS server.
* [wolfssl_version](./wolfssl_version/README.md): Bare-bones wolfSSL example.

Examples have been most recently confirmed operational on the
[Arduino IDE](https://www.arduino.cc/en/software) 2.3.6.

For examples on other platforms, see the [IDE directory](https://github.com/wolfssl/wolfssl/tree/master/IDE).
Additional wolfssl examples can be found at [wolfSSL/wolfssl-examples](https://github.com/wolfSSL/wolfssl-examples/).

## Testing

See GitHub workflow files:

- [wolfssl/arduino.yml](https://github.com/wolfssl/wolfssl/blob/master/.github/workflows/arduino.yml) Arduino CI Build (1 of 4) wolfssl
- [wolfssl-examples/arduino-release.yml](https://github.com/wolfssl/wolfssl-examples/blob/master/.github/workflows/arduino-release.yml) Arduino CI Build (2 of 4) Release Arduino wolfSSL for Local Examples
- [wolfssl-examples/arduino.yml](https://github.com/wolfssl/wolfssl-examples/blob/master/.github/workflows/arduino.yml) Arduino CI Build (3 of 4) Latest wolfSSL for Local Examples
- [Arduino-wolfSSL](https://github.com/wolfssl/Arduino-wolfSSL/blob/master/.github/workflows/arduino.yml) Arduino CI Build (4 of 4) Arduino-wolfSSL

Note that the local files here are copied to other repositories as needed from the workflow jobs:

- `arduino_install.sh` Local copy of https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
- `board_list.txt`  Comprehensive known tested boards, used by GitHub actions in wolfssl,  Arduino-wolfssl, and this repo.
- `board_list_v5.8.2_.txt`  Release v5.8.2 known tested boards, used by GitHub actions in wolfssl,  Arduino-wolfssl, and this repo.
- `compile-all-examples.sh`  Compile all examples for all sketches in current directory.

## Using wolfSSL

The typical include will look something like this:

```
#include <Arduino.h>

 /* wolfSSL user_settings.h must be included from settings.h
  * Make all configurations changes in user_settings.h
  * Do not edit wolfSSL `settings.h` or `config.h` files.
  * Do not explicitly include user_settings.h in any source code.
  * Each Arduino sketch that uses wolfSSL must have: #include "wolfssl.h"
  * C/C++ source files can use: #include <wolfssl/wolfcrypt/settings.h>
  * The wolfSSL "settings.h" must be included in each source file using wolfSSL.
  * The wolfSSL "settings.h" must appear before any other wolfSSL include.
  */
#include <wolfssl.h>

/* settings.h is typically included in wolfssl.h, but here as a reminder: */
#include <wolfssl/wolfcrypt/settings.h>

/* Any other wolfSSL includes follow:*
#include <wolfssl/version.h>
```

## Configuring wolfSSL

See the `user_settings.h` in the Arduino library `wolfssl/src` directory. For Windows users this is typically:

```
C:\Users\%USERNAME%\Documents\Arduino\libraries\wolfssl\src
```

WARNING: Changes to the library `user_settings.h` file will be lost when upgrading wolfSSL using the Arduino IDE.

## Troubleshooting

If compile problems are encountered, for example:

```
ctags: cannot open temporary file : File exists
exit status 1

Compilation error: exit status 1
```

Try deleting the Arduino cache directory:

```
C:\Users\%USERNAME%\AppData\Local\arduino\sketches
```

For VisualGDB users, delete the project `.vs`, `Output`, and `TraceReports` directories.

## More Information

For more details, see [IDE/ARDUINO/README.md](https://github.com/wolfSSL/wolfssl/blob/master/IDE/ARDUINO/README.md)
