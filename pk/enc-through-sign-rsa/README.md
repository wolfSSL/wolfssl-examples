An example that takes a simulated aes key (a string of 32-bytes / 256-bits)
and encrypts it using RSA SSL Sign in wolfSSL. Keep in mind this is not a TRUE
RSA ENCRYPT and will likely not inter-op with other libraries that offer a
RSA_PRIVATE_ENCRYPT type API.

This is a true SIGN operation.

However this will achieve, for fixed-length inputs, an encrypt via sign of the
input data.

Then using a verify operation you can un-pad and recover the original input
using the public key.

This example was created as result of a question asked on our public forums here:
https://www.wolfssl.com/forums/topic1117-does-wolfssl-support-using-rsa-private-key-to-encrypt-object-data.html


------------------------

BUILDING:

configure wolfSSL library (https://wolfssl.com/download/)
                          (https://github.com/wolfssl/wolfssl.git)
with this option:

```
./configure CFLAGS="-DUSE_CERT_BUFFERS_2048"
make
sudo make install
```

Once the wolfSSL libraries are configured and installed the from this directory
run the build.sh script which will generate the two applications

rsa-private-encrypt-app
rsa-public-decrypt-app

------------------------

USING:

```
./rsa-private-encrypt-app
```

This will output the file "encryptedAesKey"
which will contain the RSA "signed" (encrypted and padded) AES KEY that we
specified on line 33 of the application "rsa-private-encrypt-app.c".

```
33     const byte in[] = "Thisismyfakeaeskeythatis32bytes!";
```

Now run

```
./rsa-public-decrypt-app
```

This will open the file specified on line 32 of the application
"rsa-public-decrypt-app.c" and "verify" (unpad and decrypt) the file using the
public RSA key.

```
 32     char fName[] = "encryptedAesKey";
```

-----------------------

If you have any questions/concerns/feedback please contact wolfSSL at info@wolfssl.com
or support@wolfssl.com anytime!


