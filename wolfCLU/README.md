This is the wolfSSL:  Command Line Utility (wolfCLU).
To use this feature, please configure wolfssl with the following line:

'''
./configure --enable-pwdbased --enable-opensslextra
'''

The pwdbased is for password based encryption allowing the user
to specify a unique password known only to him/her self and the
recipient of the encrypted file.

The opensslextra provides utility for a hex to binary conversion of
hexidecimal values.

other features that can be included when configuring wolfssl for
encryption or decryption are:

        --enable-camellia
        --enable-des3

        --enable-blake2
        --enable-sha512
        --enable-fortress

then run "./configure", "make", and "make install" for wolfssl root directory
before using wolfCLU

After wolfssl is properly installed, to install wolfCLU:
In the root directory (wolfssl-examples/wolfCLU) enter the following commands:
    ./autogen.sh
    ./configure
    make
    sudo make install

Now you should be able to use the wolfssl command line tool.  To verify type:
    wolfssl -h

If everything worked, you should see the wolfssl help page.

Thank you and have fun!
