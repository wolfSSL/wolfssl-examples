# Posix sockets: TLS client/server example

This example shows how to use wolfSSL TLS sockets over RIOT-OS POSIX sockets


## License

This example is distributed under the terms of GNU GPLv2.

## SOCK vs. Socket

This example is configured to use wolfSSL on RIOT-OS over POSIX sockets (LWIP).


## Fast configuration (Between RIOT instances)

### Prepare the bridge interface linking two tuntap

```bash
    ./../../dist/tools/tapsetup/tapsetup --create 2
```

## Testing

### Prerequisite

Clone RIOT-OS in a directory on your filesystem.

### Run the server
```bash
$ make all RIOTBASE=/path/to/riot-os; PORT=tap1 make term
> ip
```
*copy the server address*

```bash
> tlss
```
### Run the client

The default IP address assigned to all nodes is 192.168.7.2. Modify the default address in main.c and re-compile the client application after starting the server.


```bash
$ PORT=tap0 make term
> tlsc <IPv6's server address[%netif]>
```

### Testing against host endpoints

Riot-to-host can be tested against wolfSSL client/server. The default IP address assigned to the
RIOT node in native mode is "192.168.7.2".

#### Testing TLS server on node

```bash
$ make term
```

From another console, assign address to the tap0 device on the host:
```bash
$ ifconfig tap0 192.168.7.1/24 up
```
From the RIOT native node, start the server:
```bash
> tlss
```

On the host, connect using wolfSSL example client
```bash
$ wolfssl/examples/client/client -h 192.168.7.2
```

#### Testing TLS client on node

```bash
$ make term
```

From another console, assign address to the tap0 device on the host:
```bash
$ ifconfig tap0 192.168.7.1/24 up
```


Start the wolfSSL example server  (-b is to listen on all interfaces, -d disables client certificate verification)
```bash
$ wolfssl/examples/server/server -b -d
```

From the RIOT native node, start the client:
```bash
> tlsc 192.168.7.1
```

