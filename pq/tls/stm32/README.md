# wolfSSL Post-Quantum Cryptography Example

This directory contains:

- server-tls-uart.c: A server application that can perform a completely
  quantum-safe TLS 1.3  connection over UART via a USB to TTL serial cable.
- client-tls-uart.c: A client application that can perform a completely
  quantum-safe TLS 1.3 connection over UART via a USB to TTL serial cable.
- wolfssl_stm32_pq: An STM32CubeIDE project that has already been configured to
  work with the applications above to do TLS 1.3 connections as both the server
  and client using KYBER_LEVEL1 KEM for key establishment.

# Prerequisites

Support for quantum-safe key establishment on STM32 platforms in STM32CubeIDE
is new and has not made it into a release as of the time of the writing of this
document.  Support will start as of release 5.3.0.

Please see README.md in the parent of this directory for insructions on how to
build and install liboqs and wolfSSL on your linux machine. They will be
required to build the applications on the linux side that will talk to the
STM32 board over UART.

## Building the Applications for the Linux Side

```
$ make
```

## Configuring the STM32CubeIDE Project

Please see the following video for highlights on how to configure wolfSSL to
enable our PQM4 integration.

https://www.youtube.com/watch?v=OK6MKXYiVBY

Detailed instructions on how to build a project with wolfssl can be found at:

https://github.com/wolfSSL/wolfssl/tree/master/IDE/STM32Cube

1. Follow instructions at the link above and make sure you can perform "Example
   for TLS v1.3 over UART".
2. Run the `setup_kyber.sh` script in this directory. It will clone relevant
   git repos from github and generate the directories and files you need.
3. Drag and drop the include files into `Inc` and the `pqm4_kyber512` directory
   into `Core`.
4. Turn on the PQM4 integration.
   - Open the .ioc file
   - Click "Software Packs"
   - Click on the wolfSSL.I-CUBE-wolfSSL.<version> that appears.
   - In the Configuration box that appears, set "PQM4 Post-Quantum Integration
     Support" to "True"
   - Close the .ioc file and generate code.
   - Build your project.

For your reference, `wolfssl_stm32_pq` is a minimal starting project that has
many things already configured. If you have a NUCLEO-F446ZE evaluation board
Then you can skip step 1 and 4 and just use that.

Once you have successfully built and flashed the project, the STM32 terminal
should should display the following menu:

```
                                MENU

        t. wolfCrypt Test
        b. wolfCrypt Benchmark
        l. wolfSSL TLS Bench
        e. Show Cipher List
        s. Run TLS 1.3 Server over UART
        c. Run TLS 1.3 Client over UART
Please select one of the above options:
```

## Benchmarks to Compare Against Conventional Algorithms

On the STM32 console terminal, press the b key to run the benchmarks.

Here is some sample output taken on a NUCLEO-F446ZE at 168MHz using SP Math
with Assembly Optimizations:

```
Running wolfCrypt Benchmarks...
wolfCrypt Benchmark (block bytes 1024, min 1.0 sec each)
RNG                  1 MB took 1.004 seconds,    1.070 MB/s
AES-128-CBC-enc      1 MB took 1.000 seconds,    1.172 MB/s
AES-128-CBC-dec      1 MB took 1.008 seconds,    1.187 MB/s
AES-192-CBC-enc      1 MB took 1.000 seconds,    1.001 MB/s
AES-192-CBC-dec      1 MB took 1.004 seconds,    0.997 MB/s
AES-256-CBC-enc    900 KB took 1.007 seconds,  893.744 KB/s
AES-256-CBC-dec    900 KB took 1.004 seconds,  896.414 KB/s
AES-128-GCM-enc     75 KB took 1.094 seconds,   68.556 KB/s
AES-128-GCM-dec     75 KB took 1.094 seconds,   68.556 KB/s
AES-192-GCM-enc     75 KB took 1.118 seconds,   67.084 KB/s
AES-192-GCM-dec     75 KB took 1.117 seconds,   67.144 KB/s
AES-256-GCM-enc     75 KB took 1.134 seconds,   66.138 KB/s
AES-256-GCM-dec     75 KB took 1.130 seconds,   66.372 KB/s
GMAC Small          75 KB took 1.008 seconds,   74.405 KB/s
CHACHA               4 MB took 1.004 seconds,    4.426 MB/s
CHA-POLY             3 MB took 1.000 seconds,    2.905 MB/s
POLY1305            12 MB took 1.000 seconds,   12.183 MB/s
SHA-256              3 MB took 1.000 seconds,    2.832 MB/s
HMAC-SHA256          3 MB took 1.000 seconds,    2.808 MB/s
RSA     2048 public         78 ops took 1.016 sec, avg 13.026 ms, 76.772 ops/sec
RSA     2048 private         4 ops took 1.836 sec, avg 459.000 ms, 2.179 ops/sec
DH      2048 key gen         5 ops took 1.196 sec, avg 239.200 ms, 4.181 ops/sec
DH      2048 agree           6 ops took 1.439 sec, avg 239.833 ms, 4.170 ops/sec
ECC   [      SECP256R1]   256 key gen       113 ops took 1.000 sec, avg 8.850 ms, 113.000 ops/sec
ECDHE [      SECP256R1]   256 agree          54 ops took 1.008 sec, avg 18.667 ms, 53.571 ops/sec
ECDSA [      SECP256R1]   256 sign           78 ops took 1.019 sec, avg 13.064 ms, 76.546 ops/sec
ECDSA [      SECP256R1]   256 verify         38 ops took 1.012 sec, avg 26.632 ms, 37.549 ops/sec
kyber_level1-kg         62 ops took 1.004 sec, avg 16.194 ms, 61.753 ops/sec
kyber_level1-ed         28 ops took 1.043 sec, avg 37.250 ms, 26.846 ops/sec
Benchmark complete
```

## Quantum safe TLS 1.3 Connection Over UART (Linux server, STM32 client)

You will need to use sudo as the USB serial device requires root access.
In a terminal, execute the server:

```sh
$ sudo ./server-tls-uart
```

On the STM32 console terminal, press the c key to run the client.

## Quantum safe TLS 1.3 Connection Over UART (STM32 server, Linux client)

On the STM32 console terminal, press the s key to run the server. This will put
the STM32 board into an infinite loop waiting for a client to connect over UART.
If you pressed the s key by accident, simply reset the board to bring yourself
back to the menu prompt.

You will need to use sudo as the USB serial device requires root access.
In your Linux terminal, execute the client:

```sh
$ sudo ./client-tls-uart
```

## Expected Output

The server should have the following output:

```
Running TLS 1.3 server...
Waiting for client
TLS Accept handshake done
Read (0): Testing 1, 2 and 3

Sent (0): Testing 1, 2 and 3
```

The client should have the following output:

```
Using quantum-safe KYBER_LEVEL1.
TLS Connect handshake done
Sending test string
Sent (0): Testing 1, 2 and 3

Read (0): Testing 1, 2 and 3
```
