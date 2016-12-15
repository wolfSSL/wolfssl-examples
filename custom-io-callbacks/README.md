This is an example to demonstrate how the custom IO callbacks can be used to
facilitate a TLS connection using any medium. Here we chose the medium of files.

Other mediums might be:
USB Serial connection
Bluetooth
RFID
Wifi
Ethernet

To name a few.

These examples use the wolfSSL Custom IO Callbacks to read and write to the file
system and perform a successful handshake.

The configuration used for these examples was:
./configure --enable-debug

Debug was enabled in case a user wishes to use the verbose flag to see what is
happening in real time:

Usage examples:

From the file-server directory:
./start-server
./start-server -v
./start-server -verbose

From the file-client directory:
./start-client
./start-client -v
./start-client -verbose

(-v and -verbose accomplish the same thing)


SCRIPTS: The scripts provided attempt to make testing easier.

file-server/check.sh
    - starts the server in a background shell and runs the client
USAGE:
    ./check.sh
    ./check.sh -v
    ./check.sh -verbose

file-client/clean-io-files.sh
    - If something happens and there is leftover junk in the io files run this
      script to quickly cleanup before next round of testing.
USAGE:
    ./clean-io-files.sh
