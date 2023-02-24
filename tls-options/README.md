# wolfSSL TLS with Some Options Example

This example implements a simple echo client and server that uses TLS with some options. 

## Building

You need to have wolfSSL installed on your computer prior to building.

To compile these programs use `make` in this directory.

## Running

### Session Tickets and Resumption

Here is an example of suspending a session once started and resuming the session later.

1. Establish the first session with `client-tls-session`.  
This program outputs a session ticket as a binary file.
2. Resume a previously interrupted session with `client-tls-resume`.

#### Make session ticket

On one console run the server, this should be executed first or the handshake will fail.

You can use them as a server:  
- TLS 1.2: ../tls/server-tls
- TLS 1.3: ../tls/server-tls13  

These are in different directories, so you need to use `make` again.

```sh
cd ../tls && make
./server-tls13 
```

Then in another terminal run the client:

```sh
./client-tls-session
```

You will be able to send a message from client to server.  

Sending "break" as a message to the server will generate "session.bin"


#### Resume

You can resume earlier session by doing:

```sh
./client-tls-resume
```

"session.bin" is referenced as a session ticket.   
If it doesn't exist or is invalid, this program will start a new session.

You will be able to send a message from client to server.  

Sending "break" as a message to the server will break the session.  
If you use TLS 1.3 server, You can resume many times.

### Peer Authentication

#### Enable peer authentication

You can choose peer authentication mode using:

```sh
./server-tls-peerauth -a <Peer auth mode>
```

```sh
./client-tls-peerauth -a <Peer auth mode>
```

Peer auth mode:
- NONE (Server default)
- PEER (Client default)
- FAIL_IF_NO_PEER_CERT
- FAIL_EXCEPT_PSK

See below for details.  
https://www.wolfssl.com/documentation/manuals/wolfssl/group__Setup.html#function-wolfssl_set_verify

If you specify the mode, myVerify() will call and display information about the certificate.

#### Use special verify mode

You can choose verify mode using:

```sh
./server-tls-peerauth -m <Verify mode>
```

```sh
./client-tls-peerauth -m <Verify mode>
```

Verify mode:
- OVERRIDE_ERROR
- FORCE_FAIL
- USE_PREVERIFY (default)
- OVERRIDE_DATE_ERR

If you want to use default cert files for authentication testing in server-tls-peerauth.c, please specify OVERRIDE_ERROR option.  

Because self-signed error occurs.

#### Specify options simultaneously

You can specify some options simultaneously.

Example:

```sh
./server-tls-peerauth \
    -a <Peer auth mode> -m <Verify mode>
```

```sh
./client-tls-peerauth \
    -a <Peer auth mode> -m <Verify mode>
```

## Cleaning Up

You can remove executable files by doing:

```sh
make clean
```

