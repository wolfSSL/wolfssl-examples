Configure and install wolfSSL with these options:

./configure --disable-shared --disable-asn --disable-filesystem \
    --enable-cryptonly --enable-sp=smallrsa2048 --enable-sp-math \
    --disable-dh --disable-ecc --disable-sha224 --enable-rsavfy
make
make install

(if any build issues due to previous installations please run 'ldconfig`)

To compile without Makefile:

gcc -Os -o verify verify.c /usr/local/lib/libwolfssl.a


To verify the signature with the message:

./verify


Best wishes in all your testing!

- The wolfSSL Team

