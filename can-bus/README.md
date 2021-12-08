# wolfSSL CAN Bus Example

This example implements a simple echo client and server that uses TLS over a CAN bus using [ISO-TP](https://en.wikipedia.org/wiki/ISO_15765-2) as a transport protocol. This is because the raw CAN bus protocol can only support payloads of up to 8 bytes. The example requires Linux to run but can modified to work on any setup that uses CAN bus.

## Building

You need to have wolfSSL installed on your computer prior to building. If you haven't already you will also need to get the git submodules for this tree, you can do this by using `git submodule update --init`. A simple `make` will then build the source files.

To generate the required SSL certificates use `./generate_ssl.sh`.

## Setting Up

If you do not have a physical CAN bus between too machines you can use the virtual CAN bus which is a Linux kernel module. This behaves just like a real CAN bus with a similar bandwidth. To enable this run the following commands:

```sh
modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set vcan0 up
```

## Running

On one console run the server, this should be executed first or the handshake will fail. This is executed using:

```sh
server vcan0
```

Then in another terminal run the client:

```sh
server vcan0
```

On the client you will see (byte numbers will vary):

```
Sending 164 bytes
Receiving 95 bytes
Receiving 954 bytes
Receiving 338 bytes
Receiving 9 bytes
Sending 75 bytes
Sending 6 bytes
Sending 45 bytes
Receiving 6 bytes
Receiving 45 bytes
SSL handshake done!
```

And on the server:

```
Receiving 164 bytes
Sending 95 bytes
Sending 954 bytes
Sending 338 bytes
Sending 9 bytes
Receiving 75 bytes
Receiving 6 bytes
Receiving 45 bytes
Sending 6 bytes
Sending 45 bytes
SSL handshake done!
```

Once you see the message "SSL handshake done!" on both consoles you can enter text into the client console. When you hit "enter" this will be sent to the server via the TLS encrypted CAN bus and will echo there.

For example, on the client if we type "Hello world, this is a TLS test!":

```
Hello world, this is a TLS test!

Sending: Hello world, this is a TLS test!


Sending 62 bytes
Message sent
```

The server will echo:

```
Receiving 62 bytes


Got message: Hello world, this is a TLS test!
```

## Cleaning Up

If you wish to disable the virtual CAN bus you can turn it off by doing:

```sh
sudo ip link set vcan0 down
```

