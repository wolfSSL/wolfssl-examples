# wolfSSL CAN Bus Example

This example implements a simple echo client and server that uses TLS over a CAN bus using [ISO-TP](https://en.wikipedia.org/wiki/ISO_15765-2) as a transport protocol. This is because the raw CAN bus protocol can only support payloads of up to 8 bytes. The example requires Linux to run but can modified to work on any setup that uses CAN bus.

## Building

You need to have wolfSSL installed on your computer prior to building, this will need to be built with `WOLFSSL_ISOTP` defined to provide ISO-TP functionality.

In the `wolfssl` directory:
```sh
$ ./autogen.sh (this step might not be required if the configure script already exists)
$ ./configure CFLAGS="-DWOLFSSL_ISOTP"
$ make all
$ sudo make install
```

Then in the `wolfssl-examples/can-bus` directory:
```sh
$ ./generate_ssl.sh
$ make
```

The `client` and 'server` applications will be available in the `wolfssl-examples/can-bus' directory, as well as the required SSL certificates.

## Setting Up

If you do not have a physical CAN bus between too machines you can use the virtual CAN bus which is a Linux kernel module. This behaves just like a real CAN bus with a similar bandwidth. To enable this run the following commands:

```sh
$ sudo modprobe vcan
$ sudo ip link add dev vcan0 type vcan
$ sudo ip link set vcan0 up
```

## Running

On one console run the server, this should be executed first or the handshake will fail. This is executed using:

```sh
$ server vcan0
```

Then in another terminal run the client:

```sh
$ client vcan0
```

On both ends you will see:

```
SSL handshake done!
```

Once you see the message "SSL handshake done!" on both consoles you can enter text into the client console. When you hit "enter" this will be sent to the server via the TLS encrypted CAN bus and will echo there.

For example, on the client if we type "Hello world, this is a TLS test!":

```
Hello world! This is a CAN bus test!
Sending: Hello world! This is a CAN bus test!

Message sent
```

The server will echo:

```
Got message: Hello world! This is a CAN bus test!
```

## Cleaning Up

If you wish to disable the virtual CAN bus you can turn it off by doing:

```sh
$ sudo ip link set vcan0 down
```

