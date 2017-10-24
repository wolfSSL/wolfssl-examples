# Example demonstrating signing a firmware image using ECC

## Building

### Build and install wolfSSL

```
./configure --enable-ecc && make && sudo make install
```

### Build Example

```
make
```

## Usage

```
./eccsign
Firmware Signature 0: Ret 0, HashLen 32, SigLen 71
Firmware Signature 1: Ret 0, HashLen 32, SigLen 71
Firmware Signature 2: Ret 0, HashLen 32, SigLen 71
Firmware Signature 3: Ret 0, HashLen 32, SigLen 72
Firmware Signature 4: Ret 0, HashLen 32, SigLen 71
Firmware Signature 5: Ret 0, HashLen 32, SigLen 71
Firmware Signature 6: Ret 0, HashLen 32, SigLen 71
Firmware Signature 7: Ret 0, HashLen 32, SigLen 70
Firmware Signature 8: Ret 0, HashLen 32, SigLen 70
Firmware Signature 9: Ret 0, HashLen 32, SigLen 70
```
