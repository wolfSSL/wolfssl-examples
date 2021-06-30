These PKCS12 examples need wolfSSL to be built using:

   ./configure --enable-pkcs12 --enable-pwdbased --enable-des3 --enable-keygen --enable-certgen
   make
   sudo make install


How to use pkcs12-example.c
1) Compile wolfSSL then run 'make' from this directory.

2) Run the excecutable as follows:

        ./pkcs12-example

3)  Running 'make clean' will delete the excecutable as well as any created
    files. Making sure that the only files left are 'pkcs12-example.c',
    'Makefile', 'test-servercert.p12'and 'README'.


How to use pkcs12-create-example

1) Compile wolfSSL then run 'make' from this directory.

2) A new key and certificate is generated then output into the output.p12 pkcs12
   bundled created. The default password is "test password"

      ./pkcs12-create-example
