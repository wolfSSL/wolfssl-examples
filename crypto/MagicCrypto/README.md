# MagicCrypto example usage with wolfSSL
Place this folder into wolfssl/MagicCrypto. You will need to compile wolfSSL with:
./configure --enable-ariagcm --disable-shared --enable-cryptocb --enable-all && make -j16 src/libwolfssl.la

Then simply doing `make` in the MagicCrypto folder will produce a client and server example. This uses some certificates from the wolfSSL repo. To run, simply start `./server` and `./client 127.0.0.1` in the MagicCrypto folder.
