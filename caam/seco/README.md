Example use cases when compiling wolfSSL with --enable-caam=seco

## Setup Environment

Setup the location to SECO HSM and NVM library, cryptodev header (if wolfSSL is built to look for it), wolfSSL library path, and libz install path. The following is examples of setting the environment variables:

export HSM_DIR=/home/user/imx-seco-libs/export/usr
export CRYPTODEV_DIR=/home/user/build-xwayland/sysroots-components/aarch64/cryptodev-linux/usr/include
export WOLFSSL_PATH=/home/user/wolfssl-install
export ZLIB_DIR=/home/user/zlib-aarch64-install

## Building wolfSSL
This is an example configure for building wolfSSL
./configure --host=aarch64-poky-linux --with-libz=$ZLIB_DIR --with-seco=$HSM_DIR --enable-caam=seco --enable-cmac --enable-aesgcm --enable-aesccm --enable-keygen CPPFLAGS="-DHAVE_AES_ECB -I$CRYPTODEV_DIR" --enable-devcrypto=seco --enable-curve25519 --enable-static --prefix=$WOLFSSL_PATH

## Additional cryptodev-linux Examples
Examples for use in conjunction with SECO are in the cryptodev directory

## Arguments
The first argument to the examples can be a 0 or a 1. 1 is used for creating a new entry and 0 for opening an existing one.
The second argument is the KeyID.
The third argument is the Keystore ID.
    
The KeyID should be 0 if creating it for the first time.    
    
An example of running aes-cbc would be:    

```
#create new keystore and use arbitrary keystore ID of 9
/tmp/aes-cbc 1 0 9
[ 46.049273] SECO fp open
[ 46.055038] SECO setup memory access addr 960A0000 len 66560
[ 46.061494] SECO memreg permissions set
[ 46.066412] SECO fp open
[ 46.070173] SECO setup memory access addr 960C0000 len 66560
[ 46.076634] SECO memreg permissions set
Key ID: 581588797
in = 0xffffe9138750
out = 0xffffe91386b8
cipherTxt = 0xffffe[ 46.108136] seco_mu seco_mu2: seco_mu2_ch1: Interrupted by signal
91386d8
Encrypting : 74657374206D65737361676520746F20
Cipher text: E7241091DEF6ACFEE558AA23318BCE75
Decrypted : 74657374206D65737361676520746F20
```
    
    
Then to reopen and run using the previously created key

```
#reading key ID from the previous run, re-run the example, this time opening the existing keystore
root@imx8qxpc0mek:~# /tmp/aes-cbc 0 581588797 9
[ 80.480629] SECO fp open
[ 80.484912] SECO setup memory access addr 960A0000 len 66560
[ 80.491351] SECO memreg permissions set
[ 80.498053] SECO fp open
[ 80.501851] SECO setup memory access addr 960C0000 len 66560
[ 80.508357] SECO memreg permissions set
Key ID: 581588797[ 80.516419] seco_mu seco_mu2: seco_mu2_ch1: Interrupted by signal

in = 0xffffc8928500
out = 0xffffc8928468
cipherTxt = 0xffffc8928488
Encrypting : 74657374206D65737361676520746F20
Cipher text: E7241091DEF6ACFEE558AA23318BCE75
Decrypted : 74657374206D65737361676520746F20
```
   
This will create the keystores at /etc/seco_hsm . So far none of the keystores that wolfSSL creates are PERMENANT type ones, just either persistent or ephemeral. So to reset the keystore root can do "rm -rf /etc/seco_hsm" (as suggested by one of NXP's manuals on it) and reset the device.
