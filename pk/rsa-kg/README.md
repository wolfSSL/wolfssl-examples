How to use rsa-kg-sv.c

1) a. Compile wolfSSL with  ./configure --enable-keygen, run
      'make', and then install by typing 'sudo make install'.
   b. In the pk/rsa-kg directory run the Makefile by typing 'make'.
2)  run the executable, for help run with -help. Basic command is as follows:

        ./rsa-kg-sv <options>

        Key sizes supported, in bits: 1024-4096.

    With option -load-key, use the key in: rsa-key.h
        NOTE: on error, the key and digest will be displayed so that they can be
        copied into rsa-key.h replacing the existing values.

3)  Running 'make clean' will delete the executable and object files.


How to use rsa-kg.c

1) a. Compile wolfSSL with  ./configure --enable-keygen, run
      'make', and then install by typing 'sudo make install'.
   b. In the pk/rsa-kg directory run the Makefile by typing 'make'.
2)  run the executable, for help run with -help. Basic command is as follows:

        ./rsa-kg <options>

        Key sizes supported, in bits: 1024-4096.

        Specify the name of the output files with -priv and -pub.

3)  Running 'make clean' will delete the executable and object files.

