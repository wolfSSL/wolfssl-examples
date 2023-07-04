How to use camellia-encrypt.c

1) a. Compile wolfSSL with ./configure --enable-pwdbased --enable-camellia, run
      'make', and then install by typing 'sudo make install'.
   b. In the crypto/camellia directory run the Makefile by typing 'make'.
2)  Make a file to encode. Can be any file (ex. .txt .in .out .file etc.)
3)  run the executable, for help run with -h flag. Basic command is as follows:

        ./camellia-encrypt <option> <KeySize> -i <input.file> -o <output.file>

        KeySize examples: 128, 192, 256

    typing -e for option will encrypt the input.file onto the output.file.
    typing -d for option will decrypt the input.file onto the output.file.
        NOTE: When decrypting make sure the key is the same used for the
        encryption, otherwise it won't decode correctly. Which is the idea.
        Only those with the key will be able to decode the message. If no
        key is entered into the command line, it will use "0123456789abcdef"
        by default.

4)  Running 'make clean' will delete the executable as well as any created
    files. Making sure that the only files left are 'camellia-encrypt.c',
    'Makefile', and 'README'.
