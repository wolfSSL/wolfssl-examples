Configure and install wolfSSL with these options:

./configure --enable-stable
CFLAGS="-DWOLFSSL_PUBLIC_MP"
make
make install

(if any build issues due to previous installations please run 'ldconfig`)

To compile without Makefile:

gcc -Os -o verify verify.c /usr/local/lib/libwolfssl.a


To verify the signature with the message:

./verify


Best wishes in all your testing!

- The wolfSSL Team

