To run ./test_aescbc_3des_cam.sh you need to have cyassl configured with the 
following:

            --enable-pwdbased 
            --enable-opensslextra 
            --enable-camellia 
            --enable-des3
Additionally You will need to go to:

<wolfssl command line utility root>/tests/somejunk

and execute each of the .sh scripts to generate the respective test files.

