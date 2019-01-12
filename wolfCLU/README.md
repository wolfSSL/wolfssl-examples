# wolfCLU

This is the wolfSSL: Command Line Utility (wolfCLU).

## wolfSSL Install

To use this feature, please configure and install wolfssl with the following commands:

    ./configure --enable-pwdbased --enable-opensslextra --enable-keygen \
                --enable-ed25519 --enable-certgen \
     && make && make check

If that succeeds, run:

    sudo make install

`--enable-pwdbased` is for password based encryption allowing the user
to specify a unique password known only to him/her self and the
recipient of the encrypted file.

`--enable-opensslextra` provides utility for a hex to binary conversion of
hexidecimal values.

`--enable-base64encode` enables Base64 encoding (not on by default)

`--enable-keygen` enables key generation (not on by default)


Additional features that can be included when configuring wolfssl for
encryption or decryption are:

        --enable-camellia
        --enable-des3
        --enable-blake2
        --enable-sha512
        --enable-fortress

Additional features that can be included when configuring wolfssl for
key generation are:



##wolfCLU Install

After wolfssl is installed, install wolfCLU.  In the directory
`wolfssl-examples/wolfCLU` enter the following commands:

    ./autogen.sh
    ./configure
    make
    (optionally) make check OR make test
    sudo make install

Now you should be able to use the wolfssl command line tool.  To verify type:

    wolfssl -h

If everything worked, you should see the wolfssl help page.

## Example Usages

### Base64

#### Encode

```
./wolfssl -hash base64enc -in README.md > README_encoded.md
```

#### Decode

```
./wolfssl -hash base64dec -in README_encoded.md
```

#### X509

```
wolfssl -x509 -inform pem -in testing-certs/ca-cert.pem -outform der -out outputfilename.der
wolfssl -x509 -inform der -in testing-certs/ca-cert.der -outform pem -out outputfilename.pem
```

## Contacts

Please contact support@wolfssl.com with any questions or comments

## License

Copyright (c) 2006-2017 wolfSSL Inc.
