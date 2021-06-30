How to use dh-pg-ka.c

1) a. Compile wolfSSL with  ./configure --enable-keygen, run
      'make', and then install by typing 'sudo make install'.
      NOTE: optionally compile with:
          C_EXTRA_FLAGS="-DHAVE_FFDHE_2048 -DHAVE_FFDHE_3072 -DHAVE_FFDHE_4096"
   b. In the pk/dh-pg directory run the Makefile by typing 'make'.
2)  run the executable, for help run with -help. Basic command is as follows:

        ./dh-pg-ka <options>

        Parameter sizes supported, in bits: 2048, 3072, 4096.

    With option -ffdhe, use the pre-defined standardized FFDHE parameters.
        NOTE: no parameters will be generated.
    With option -load, use the parameters in: dh-param.h
        NOTE: on error, the parameters will be displayed so that they can be
        copied into dh-params.h replacing the existing values.
    Note: 4096-bit parameters cannot be generated - not supported by wolfSSL.

4)  Running 'make clean' will delete the executable and object files.


