This is the wolfSSL:  Command Line Utility (wolfssl CLU).
To use this feature, please configure cyassl with the following line:

'''
./configure --enable-pwdbased --enable-opensslextra
'''

The pwdbased is for password based encryption allowing the user
to specify a unique password known only to him/her self and the 
recipient of the encrypted file.

The opensslextra provides utility for a hex to binary conversion of 
hexidecimal values.

other features that can be included when configuring cyassl for 
encryption or decryption are:

        --enable-camellia
        --enable-des3
 
        --enable-blake2
        --enable-sha512
        --enable-fortress

then run "./configure", "make", and "make install" before using wolfssl CLU

Thank you and have fun!
