Configure and install wolfSSL with these options:

./configure --enable-ed25519
make
make install

(if any build issues due to previous installations please run 'ldconfig`)

To compile without Makefile:

gcc -o gen_key_files gen_key_files.c -lwolfssl
gcc -o sign_and_verify sign_and_verify.c -lwolfssl


To re-create the ed25519 private/public key files and update the test_keys.h
header file run these commands:

./gen_key_files
./genkeybuffers.pl

To sign a message and verify the signature run:

./sign_and_verify


Best wishes in all your testing!

- The wolfSSL Team

