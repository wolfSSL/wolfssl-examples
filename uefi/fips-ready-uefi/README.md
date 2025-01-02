wolfcrypt UEFI FIPS Ready Example Application
===================================

This application is an expansion of the `basic-uefi` project found in the
`wolfssl-examples` github repository. This application is an example on how to
setup a FIPS Ready build of wolfSSL to help kickstart development for an
eventual FIPS required UEFI applicaiton. FIPS Ready is only for evaluation and
development purposes for eventual usage and aquirement of the FIPS Certified
bundle of wolfSSL. FIPS Ready is not certified in anyway, to learn more refer to
the wolfSSL FIPS Ready user guide.

As UEFI is not POSIX there are a few functions that will need to be added in
order to for wolfSSL to properly function. These functions are like open/close
for files and then standard printing functions that wolfSSL uses. The example
will leverage gnu-efi for some of these standard operations. wolfSSL will need
access to a form of RNG/Entropy as well.

The functions in this example are not meant for production and only
serve as an example of functions needed for wolfSSL's crypto to function in
UEFI. For how the functions are setup please refer to the comments and
`utility_wolf.c`.



Requires gnu-efi. Tested with qemu and OVFM UEFI implementation.  It uses a
custom implementation for string.h functions (`string.c`) based on wolfBoot
`src/string.c`. You can customize the build by changing `user_settings.h` and
adding the relevant `.o` file into the Makefile `_OBJS` variable.



# Compile

## Pre-requisites

```
git make gcc gnu-efi
```

You will need to download and unzip the wolfSSL FIPS ready bundle.

wolfSSL FIPS Ready:
 - [bundle download](https://www.wolfssl.com/download/)
 - [user guide](https://www.wolfssl.com/documentation/manuals/wolfssl-fips-ready/)

## build

```
make

```

# Test on qemu

## Pre-requisites
```
dosfstools qemu qemu-system-x86 ovmf
```
## Create efi disk

We need to create a FAT partition to store efi application.

```
dd if=/dev/zero of=./efi.disk bs=256M count=1
sudo mkfs.vfat ./efi.disk
```

Move wolfcrypt.efi and startup.nsh into the fat32 partition
```
mkdir -p /tmp/efi
sudo mount ./efi.disk /tmp/efi -oloop
sudo cp wolfcrypt.efi /tmp/efi
sudo cp startup.nsh /tmp/efi
sudo umount /tmp/efi
```

## Run qemu

```
qemu-system-x86_64 -bios /path/to/OVMF.fd -display none -serial stdio -net none -m 256M -drive file=./efi.disk,index=0,media=disk,format=raw
```

/path/to/OVMF.fd may be /usr/share/edk2-ovmf/x64 or /usr/share/qemu

## Example output

```
UEFI Interactive Shell v2.2
EDK II
UEFI v2.70 (EDK II, 0x00010000)
Mapping table
      FS0: Alias(s):F0a:;BLK0:
          PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)
     BLK1: Alias(s):
          PciRoot(0x0)/Pci(0x1,0x1)/Ata(0x0)
Press ESC in 1 seconds to skip startup.nsh or any other key to continue.
Shell> fs0:
FS0:\> wolfcrypt.efi
status: 0x0
Image base: 0xE15E000
------------------------------------------------------------------------------
 wolfSSL version ⸵⸶6
------------------------------------------------------------------------------
error    test passed!
MEMORY   test passed!
base64   test passed!
asn      test passed!
RANDOM   test passed!
SHA-256  test passed!
SHA-3    test passed!
SHAKE128 test passed!
SHAKE256 test passed!
Hash     test passed!
HMAC-SHA256 test passed!
HMAC-SHA3   test passed!
AES      test passed!
AES192   test passed!
AES256   test passed!
RSA      test passed!
logging  test passed!
mutex    test passed!
Test complete
ret: 0
FS0:\> 
```
