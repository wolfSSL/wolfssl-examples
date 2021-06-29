Configure and install wolfSSL with these options:

./configure
make
make install

(if any build issues due to previous installations please run 'ldconfig`)

To compile without Makefile:

gcc -o sign sign.c -lwolfssl
./sign <message> > signature.h
gcc -o verify verify.c -lwolfssl


To sign a message:

./sign <message>

To verify the signature with the message:

./verify


Best wishes in all your testing!

- The wolfSSL Team

