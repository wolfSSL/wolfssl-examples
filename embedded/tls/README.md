# Examples for Embedded systems

## Building

### Build and install wolfSSL

```
./configure && make && sudo make install
```

### Build Example

```
make
gcc -o tls-sock-client tls-sock-client.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o tls-sock-server-ca tls-sock-server-ca.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o tls-sock-threaded tls-sock-threaded.c -Wall -I/usr/local/include -Os -pthread -L/usr/local/lib -lm -lwolfssl
gcc -o tls-client-server tls-client-server.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o tls-sock-server tls-sock-server.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o tls-sock-client-ca tls-sock-client-ca.c -Wall -I/usr/local/include -Os -L/usr/local/lib -lm -lwolfssl
gcc -o tls-threaded tls-threaded.c -Wall -I/usr/local/include -Os -pthread -L/usr/local/lib -lm -lwolfssl
```

### Debug

To enable debug change the Makefile to:

```
CFLAGS+=$(DEBUG_FLAGS)
#CFLAGS+=$(OPTIMIZE)
```

Build wolfSSL adding `--enable-debug` to the ./configure.

To enable using the static library change the Makefile to:

```
LIBS+=$(STATIC_LIB)
#LIBS+=$(DYN_LIB)
```

Build wolfSSL adding `--disable-shared` to the ./configure.


## Usage

### `tls-client-server`

This example demonstrates a client and server communicating through buffers (i.e. not sockets.) wolfSSL_SetIOSend() and wolfSSL_SetIORecv() are used to set the callback functions to send and receive TLS message data. Note that wolfSSL will request as many bytes as needed to process a TLS message and expects a return of WOLFSSL_CBIO_ERR_WANT_READ when no data is available. Similarly, the return code from the send function must be the number of bytes successfully dealt with or WOLFSSL_CBIO_ERR_WANT_WRITE when no bytes could be sent.

```
./tls-client-server 
Client waiting for server
Server waiting for server
Client waiting for server
Handshake complete

Client Sending:
GET /index.html HTTP/1.0


Server Received:
GET /index.html HTTP/1.0


Server Sending:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

Client Received:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>
Done
```

### `tls-threaded`

This example demonstrates a client and server, in separate threads, communicating through buffers (i.e. not sockets.) wolfSSL_SetIOSend() and wolfSSL_SetIORecv() are used to set the callback functions to send and receive TLS message data. Note that wolfSSL will request as many bytes as needed to process a TLS message and expects a return of WOLFSSL_CBIO_ERR_WANT_READ when no data is available. Similarly, the return code from the send function must be the number of bytes successfully dealt with or WOLFSSL_CBIO_ERR_WANT_WRITE when no bytes could be sent.

```
./tls-threaded 
Handshake complete
Sending:
GET /index.html HTTP/1.0


Received:
GET /index.html HTTP/1.0


Sending:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

Server Return: 0
Received:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

Client Return: 0
Done
```

### `tls-sock-client`

This example demonstrates a TLS client using sockets. The client attempts to connect to: localhost:11111. The client will downgrade to the highest version supported by both peers (wolfSSLv23_client_method().)


```
[./tls-sock-server]
./tls-sock-client
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
Sending:
GET /index.html HTTP/1.0


Receive:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>
Peer closed socket
Done
```

### `tls-sock-client-ca`

This example demonstrates a TLS client performing client authentication and using sockets. The client attempts to connect to: localhost:11111. The client will downgrade to the highest version supported by both peers (wolfSSLv23_client_method().)

```
[./tls-sock-server-ca]
./tls-sock-client-ca
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
Sending:
GET /index.html HTTP/1.0


Receive:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>
Peer closed socket
Done
```

### `tls-sock-server`

This example demonstrates a TLS server using sockets. The server accepts connections on: localhost:11111. The server will downgrade to the highest version supported by both peers (wolfSSLv23_server_method().)

```
./tls-sock-server
[./tls-sock-client]
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
Receive:
GET /index.html HTTP/1.0

Sending:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

[./tls-sock-client]
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
Receive:
GET /index.html HTTP/1.0

Sending:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

^C
```

### `tls-sock-server-ca`

This example demonstrates a TLS server performing client authentication using sockets. The server accepts connections on: localhost:11111. The server will downgrade to the highest version supported by both peers (wolfSSLv23_server_method().)

```
./tls-sock-server-ca
[./tls-sock-client-ca]
SSL version is TLSv1.2
SSL cipher suite is TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
SSL curve name is SECP256R1
Receive:
GET /index.html HTTP/1.0

Sending:
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close

<html>
<head>
<title>Welcome to wolfSSL!</title>
</head>
<body>
<p>wolfSSL has successfully performed handshake!</p>
</body>
</html>

^C
```

### `tls-server-size`

This example is useful in determining the code size of a minimal TLS server.
The example will *NOT* complete a handshake as there is no client.


## Support

For questions please email us at support@wolfssl.com.
