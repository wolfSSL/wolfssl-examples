## Getting Started

1. Put wolfSSL source files under this directory. 
   RPi-Pico/wolfssl

2. Setup pico-sdk and set PICO_SDK_PATH
    export PICO_SDK_PATH=/your/pico-sdk/path

3. cmake and make
    $ cd RPi-Pico
    $ mkdir build
    $ cd build
    $ cmake -DPICO_BOARD=pico_w ..
    $ make

4. Output is to USB serial

   