## TLS server using picoTCP via wolfSSL custom callbacks

This TLS server runs in userspace, using picoTCP as compiled-in TCP/IP stack.

`picotcp-server` has been tested on Linux using latest picoTCP master, and wolfSSL 4.5.0.


### Requirements

- PicoTCP v.1.7 or later
- wolfSSL (default configuration)
- Access to `/dev/net/tun` on the host system (typically root privileges)

### How to compile picotcp-server

- clone or download [picoTCP](https://github.com/tass-belgium/picotcp.git)
- compile picoTCP with `make ARCH=shared TAP=1 WOLFSSL=1`
- modify `PICOTCP_PATH` at the top of Makefile, pointing to the picoTCP root directory
- run `make`
 
### Testing picotcp-server

Run `picotcp-server` with no arguments. If the executable was run with the right permissions (i.e. accessing `/dev/net/tun`), a `tap0` device will be created, and the picoTCP endpoint address is set to `10.0.0.1/24`.

The TLS server listens to port 11111 by default, and can be reached using the [TLS client example](../tls/client-tls.c).

Example:

```(bash)
# Server side:
./picotcp-server

# On client side:
ip addr add dev tap0 10.0.0.2/24
ip link set tap0 up

./client-tls 10.0.0.1

```
