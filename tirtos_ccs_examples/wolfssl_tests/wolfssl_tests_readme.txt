Example Summary
----------------
This application demonstrates the wolfssl/testsuite/testsuite.test

Notes
----------------
Due to large amounts of heap needed to run these tests SHA-512 and Blake-2B have been disabled for this project.
If you would like to run them first right click the project file and select "Properties" (very bottom)

in the explorer locate and click on "ARM Compiler". In the right pane you will see a summary of flags box. at
the bottom click on "Set Additional Flags..." locate and delete the following:

--define=HAVE_CAMELLIA
--define=HAVE_CHACHA
--define=HAVE_ECC
--define=HAVE_POLY1305

then put in the following:
--define=WOLFSSL_SHA512
--define=HAVE_BLAKE2

Then build and debug this project again.
