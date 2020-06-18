An example of generating an ecdh secret

Building:
configure wolfssl with --enable-curve25519 --enable-curve448 --enable-ecc and
make install

Makefile by default uses /usr/local for LIB_PATH to find /usr/local/include
and /usr/local/lib. If you configured wolfSSL with --prefix=/my/custom/path
update the makefile LIB_PATH variable to use /my/custom/path instead.

run 'make'

Currently supports curves:

SECP256R1
CURVE25519
CURVE448

Usage:

./ecdh_gen_secret - Will ask for run-time user input
./ecdh_gen_secret 1 - no run-time user input, uses SECP256R1
./ecdh_gen_secret 2 - no run-time user input, uses CURVE25519
./ecdh_gen_secret 3 - no run-time user input, uses CURVE448

NOTE: By default the example generates keys at run time for use. If you would
prefer to use buffers see wolfcrypt/test/test.c example of importing keys with
the wc_ecc_import, wc_curve25519_import or wc_curve448_import API's (imports
from a buffer to a key structure).


Best of luck in all your testing, contact the wolfSSL support team via email at
support@wolfssl.com or through the zendesk portal at https://wolfssl.zendesk.com
if you have any questions or issues. Thanks!

- The wolfSSL Team
