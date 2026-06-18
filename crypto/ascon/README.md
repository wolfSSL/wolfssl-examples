How to use ascon-file-encrypt.c

1) a. Compile wolfSSL with ./configure --enable-pwdbased --enable-ascon --enable-experimental, run
      'make', and then install by typing 'sudo make install'.
   b. In the crypto/ascon directory run the Makefile by typing 'make'.
2)  Make a file to encode. Can be any file (ex. .txt .in .out .file etc.)
3)  run the executable, for help run with -h flag. Basic command is as follows:
        ./ascon-file-encrypt <-option> -i <input.file> -o <output.file>

    typing -e for option will encrypt the input.file onto the output.file.
    typing -d for option will decrypt the input.file onto the output.file.
        NOTE: When decrypting make sure the key is the same used for the
        encryption, otherwise it won't decode correctly. Which is the idea.
        Only those with the key will be able to decode the message. If no
        key is entered into the command line, the program will error out.

4)  Running 'make clean' will delete the executable as well as any created
    files. Making sure that the only files left are 'ascon-file-encrypt.c',
    'Makefile', and 'README.md'.
