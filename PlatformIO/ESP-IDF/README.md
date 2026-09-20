# PlatformIO wolfssl-specific ESP-IDF releases

## esp-idf-v5.3.1.20240926c.zip

This file is from https://github.com/gojimmypi/esp-idf/releases/tag/v5.3.1.20240926c

It contains the Espressif ESP-IDF with wolfSSL support for the `esp-tls` component.

See also: https://github.com/espressif/esp-idf/pull/16145

Tested with:

```ini
[env]
# See https://github.com/platformio/platform-espressif32/releases/tag/v6.8.0
# uses ESP-IDF v5.3
platform = espressif32@6.8.0

# ESP-IDF v5.3.1 (20240926c)
platform_packages = framework-espidf @ https://raw.githubusercontent.com/wolfssl/wolfssl-examples/tree/master/PlatformIO/ESP-IDF/esp-idf-v5.3.1.20240926c.zip
```
