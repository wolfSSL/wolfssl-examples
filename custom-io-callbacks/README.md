This is an example to demonstrate how the custom IO callbacks can be used to</br>
facilitate a TLS connection using any medium. Here we chose the medium: "File System".</br>

Other mediums might be: </br>
USB Serial connection </br>
Bluetooth </br>
RFID </br>
Wifi </br>
Ethernet </br>
</br>
Just to name a few.</br>
</br>
These examples use the wolfSSL Custom IO Callbacks to read and write to the file</br>
system and perform a successful handshake.</br>
</br>
The configuration used for these examples was:</br>
./configure --enable-debug</br>
</br>
Debug was enabled in case a user wishes to use the verbose flag to see what is</br>
happening in real time:</br>
</br>
Usage examples:</br>
</br>
From the file-server directory:</br>
./start-server</br>
./start-server -v</br>
./start-server -verbose</br>
</br>
From the file-client directory:</br>
./start-client</br>
./start-client -v</br>
./start-client -verbose</br>
</br>
(-v and -verbose accomplish the same thing)</br>
</br>
</br>
SCRIPTS: The scripts provided attempt to make testing easier.</br>
</br>
file-server/check.sh</br>
    - starts the server in a background shell and runs the client </br>
    </br>
USAGE:</br>
    ./check.sh</br>
    ./check.sh -v</br>
    ./check.sh -verbose</br>
</br>
file-client/clean-io-files.sh</br>
    - If something happens and there is leftover junk in the io files run this</br>
      script to quickly cleanup before next round of testing.</br>
USAGE:</br>
    ./clean-io-files.sh</br>
    </br>
