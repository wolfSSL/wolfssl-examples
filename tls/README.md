Tutorial
========

This tutorial will teach you how to install and run a basic TCP Server and Client. As well as how to incorporate CyaSSL TLS and some additional features on top of these basic examples. It is expected that you have a basic understanding of a simple tcp server/client. If not, before continueing please take a moment to look over the `server-tcp.c` and `client-tcp.c` file which contains the basic tcp server that we will be expanding upon in this tutorial.


First you will need `gcc` and `make` installed on your terminal. You can do this by opening a new terminal window and typing:

	sudo apt-get install gcc make

## Index
1. [Incorporating CyaSSL TLS](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#incorporating-cyassl-tls)
  * [Installing CyaSSL](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#installing-cyassl)
2. [Server TLS Tutorial](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#server-tls-tutorial)
  * [Basic TLS Server](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#basic-tls-server)
  * [Adding Multi-threading](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-server-multi-threading)
  * [Adding Non-blocking I/O](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-server-non-blocking-io)
3. [Client TLS Tutorial](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#client-tls-tutorial)
  * [Basic TLS Client](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#basic-tls-client)
  * [Adding Session Resumption](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-client-session-resumption)
  * [Adding Non-blocking I/O](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-client-non-blocking-io)
4. [Starting the TLS Client & Server](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#starting-the-tls-client--server)

## Incorporating CyaSSL TLS
To incorporate CyaSSL TLS into your client or server you need to first configure and install the CyaSSL library to your linux machine. After you have done that, you can then run `make` to compile the TLS versions into an executeable.

### Installing CyaSSL
+ Download and extract the CyaSSL package from [here.](http://wolfssl.com/yaSSL/Products-cyassl.html)
+ In terminal, navigate to the root of the extracted folder.
+ Type `./configure` press enter. Wait until it finishes configuring.
+ Type `make` press enter. 
+ Type `sudo make install`, this will install the CyaSSL libraries to your machine.

CyaSSL libraries should now be installed to your machine and ready to use. You can now make and run the server and client examples.

## Server TLS Tutorial

### Basic TLS Server
To begin, we will be re-writing the basic `server-tcp.c` with CyaSSL. The structure of the file will be almost identical. To begin, we will need to include the cyassl libraries with the rest of our includes at the top of the file: 

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

/* include the CyaSSL library for our TLS 1.2 security */
#include <cyassl/ssl.h>
```
Next we will add our `#define DEFAULT_PORT 11111` and the prototype for our `AcceptAndRead` function:

```c
#define DEFAULT_PORT 11111

int AcceptAndRead(CYASSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in 
    clientAddr);
```
Now we will build our `main()` function for the program. What happens here is we create a CYASSL context pointer and a socket. We then initialize CyaSSL so that it can be used. After that we tell CyaSSL where our certificate and private key files are that we want our server to use. We then attach our socket to the `DEFAULT_PORT` that we defined above. The last thing to do in the main function is to listen for a new connection on the socket that we binded to our port above. When we get a new connection, we call the `AcceptAndRead` function. The main function should look like: 

```c
int main()
{
    /* Create a ctx pointer for our ssl */
    CYASSL_CTX* ctx;

    /* 
     * Creates a socket that uses an internet IP address,
     * Sets the type to be Stream based (TCP),
     * 0 means choose the default protocol.
     */
    socklen_t sockfd   = socket(AF_INET, SOCK_STREAM, 0);
    int loopExit = 0; /* 0 = False, 1 = True */
    int ret      = 0; /* Return value */
    /* Server and client socket address structures */
    struct sockaddr_in serverAddr, clientAddr;

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* If positive value, the socket is valid */
    if (sockfd == -1) {
        printf("ERROR: failed to create the socket\n");
        return EXIT_FAILURE;        /* Kill the server with exit status 1 */        
    }

    /* create and initialize CYASSL_CTX structure */
    if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* Load server certificate into CYASSL_CTX */
    if (CyaSSL_CTX_use_certificate_file(ctx, "../certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Load server key into CYASSL_CTX */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, "../certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-key.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Initialize the server address struct to zero */
    memset((char *)&serverAddr, 0, sizeof(serverAddr)); 

    /* Fill the server's address family */
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(DEFAULT_PORT);

    /* Attach the server socket to our port */
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))
        < 0) {
        printf("ERROR: failed to bind\n");
        return EXIT_FAILURE;
    }

    printf("Waiting for a connection...\n");
    /* Continuously accept connects while not currently in an active connection
       or told to quit */
    while (loopExit == 0) {
        /* listen for a new connection, allow 5 pending connections */
        ret = listen(sockfd, 5);
        if (ret == 0) {

            /* Accept client connections and read from them */
            loopExit = AcceptAndRead(ctx, sockfd, clientAddr);
        }
    }

    CyaSSL_CTX_free(ctx);   /* Free CYASSL_CTX */
    CyaSSL_Cleanup();       /* Free CyaSSL */
    return EXIT_SUCCESS;
}
```
Now all that is left is the `AcceptAndRead` function. This function accepts the new connection and passes it off to its on file descriptor `connd`. We then create our ssl object and direct it to our clients connection. Once thats done we jump into a `for ( ; ; )` loop and do a `CyaSSL_read` which will decrypt and send any data the client sends to our `buff` array. Once that happens we print the data to the console and then send a reply back to the client letting the client know that we reicieved their message. We then break out of the loop, free our ssl and close the `connd` connection since it's no longer used. We then `return 0` which tells our loop in main that it was successful and to continue listening for new connections. 

Here is the `AcceptAndRead` with more detailed comments on what's happening.

```c
int AcceptAndRead(CYASSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in 
    clientAddr)
{
        /* Create our reply message */
    const char reply[]  = "I hear ya fa shizzle!\n";
    socklen_t         size    = sizeof(clientAddr);

    /* Wait until a client connects */
    socklen_t connd = accept(sockfd, (struct sockaddr *)&clientAddr, &size);

    /* If fails to connect,int loop back up and wait for a new connection */
    if (connd == -1) {
        printf("failed to accept the connection..\n");
    }
    /* If it connects, read in and reply to the client */
    else {
        printf("Client connected successfully\n");
        CYASSL*     ssl;

        if ( (ssl = CyaSSL_new(ctx)) == NULL) {
            fprintf(stderr, "CyaSSL_new error.\n");
            exit(EXIT_FAILURE);
        }

        /* direct our ssl to our clients connection */
        CyaSSL_set_fd(ssl, connd);

        printf("Using Non-Blocking I/O: %d\n", CyaSSL_get_using_nonblock(
            ssl));

        for ( ; ; ) {
            char buff[256];
            int  ret = 0;

            /* Clear the buffer memory for anything  possibly left over */
            memset(&buff, 0, sizeof(buff));

            /* Read the client data into our buff array */
            if ((ret = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                /* Print any data the client sends to the console */
                printf("Client: %s\n", buff);
                
                /* Reply back to the client */
                if ((ret = CyaSSL_write(ssl, reply, sizeof(reply)-1)) 
                    < 0)
                {
                    printf("CyaSSL_write error = %d\n", CyaSSL_get_error(ssl, ret));
                }
            }
            /* if the client disconnects break the loop */
            else {
                if (ret < 0)
                    printf("CyaSSL_read error = %d\n", CyaSSL_get_error(ssl
                        ,ret));
                else if (ret == 0)
                    printf("The client has closed the connection.\n");

                break;
            }
        }
        CyaSSL_free(ssl);           /* Free the CYASSL object */
    }
    close(connd);               /* close the connected socket */

    return 0;
}
```
And with that, you should now have a basic TLS server that accepts a connection, reads in data from the client, sends a reply back, and closes the clients connection. 

### Adding Server Multi-threading

### Adding Server Non-blocking I/O

## Client TLS Tutorial

### Basic TLS Client

Again, we will need to import the security library.  Just like in the server, add an `#include` statement in your client program.  Next we will need to add a global `cert` variable: 

```c
const char* cert = "../certs/ca-cert.pem";
```

Now comes changing the `ClientGreet()` function so its arguments and functions incorporate the security library.

```c
void ClientGreet(int sock, CYASSL* ssl)

if (CyaSSL_write(ssl, send, strlen(send)) != strlen(send)) {

if (CyaSSL_read(ssl, receive, MAXDATASIZE) == 0) {
```

You can think of this as, instead of just a normal read and write, it is now a “secure” read and write.  We also need to change the call to `ClientGreet()` in `main()`.  Instead of calling directly to it, we should make a call to a `Security()` that will then check the server for the correct `certs`.  To do this, change:

```c
ClientGreet(sock(fd));
```

to

```c
Security(sockfd);
```

Now we just have to make the `Security()` function. It should look something like:

```c
/* 
 * applies TLS 1.2 security layer to data being sent.
 */
int Security(int sock)
{
    CYASSL_CTX* ctx;
    CYASSL*     ssl;    /* create CYASSL object */
    int         ret = 0;

    CyaSSL_Init();      /* initialize CyaSSL */

    /* create and initiLize CYASSL_CTX structure */
    if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* load CA certificates into CyaSSL_CTX. which will verify the server */
    if (CyaSSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }
    if ((ssl = CyaSSL_new(ctx)) == NULL) {
        printf("CyaSSL_new error.\n");
        return EXIT_FAILURE;
    }
    CyaSSL_set_fd(ssl, sock);

    ret = CyaSSL_connect(ssl);
    if (ret == SSL_SUCCESS) {
        ret = ClientGreet(sock, ssl);
    }

    /* frees all data before client termination */
    CyaSSL_free(ssl);
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();

    return ret;
}
```

As you can see, this is where we make the call to “greet” the server.  This function sends its certification, `../ca-certs.pem` to the server which checks for this. If it’s there, it establishes the connection and secures the information being sent and received between the two.  Once this has been done, it frees all the data so no processes remain after the connection has been terminated.

### Adding Client Session Resumption

In case the connection to the server gets lost, and you want to save time, you’ll want to be able to resume the connection. In this example, we disconnect from the server, then reconnect to the same session afterwards, bypassing the handshake process and ultimately saving time. To accomplish this, you’ll need to add some variable declarations in `Security()`.

```c
CYASSL_SESSION* session = 0;/* cyassl session */                            
CYASSL*         sslResume;  /* create CYASSL object for connection loss */
```

Next we'll have to add some code to disconnect and then reconnect to the server. This should be added after your `ClientGreet()` call in `Security()`.

```c
/* saves the session */
session = CyaSSL_get_session(ssl);
CyaSSL_free(ssl);

/* closes the connection */
close(sock);

/* new ssl to reconnect to */
sslResume = CyaSSL_new(ctx);

/* makes a new socket to connect to */
sock = socket(AF_INET, SOCK_STREAM, 0);

/* sets session to old session */
CyaSSL_set_session(sslResume, session);

/* connects to new socket */
if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    /* if socket fails to connect to the server*/
    ret = CyaSSL_get_error(ssl, 0);
    printf("Connect error. Error: %i\n", ret);
    return EXIT_FAILURE;
}

/* sets new file discriptior */
CyaSSL_set_fd(sslResume, sock);

/* reconects to CyaSSL */
ret = CyaSSL_connect(sslResume);
if (ret != SSL_SUCCESS) {
    return ret;
}

/* checks to see if the new session is the same as the old session */
if (CyaSSL_session_reused(sslResume))
    printf("Re-used session ID\n"); 
else
    printf("Did not re-use session ID\n");

/* regreet the client */
ret = ClientGreet(sock, sslResume);
```

We will aslo have to slightly alter our last `CyaSSL_free()` call. Instead of `CyaSSL_free(ssl);` it needs to state `CyaSSL_free(sslResume);`

### Adding Client Non-blocking I/O

The first thing that has to be done in order to make a socket a non-blocking socket is to add another library to the top of the code.

```c
#include <fcntl.h>               /* nonblocking I/O library */
```

Then we will need to add a few functions and an `enum` after the `cert` variable.

```c
/*
 * enum used for tcp_select function 
 */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

static inline int tcp_select(int socketfd, int to_sec)
{
    fd_set recvfds, errfds;
    int nfds = socketfd + 1;
    struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
    int result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    if (result == 0)
        return TEST_TIMEOUT;
    else if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds))
            return TEST_RECV_READY;
        else if(FD_ISSET(socketfd, &errfds))
            return TEST_ERROR_READY;
    }

    return TEST_SELECT_FAIL;
}
int NonBlockConnect(CYASSL* ssl)
{
    int ret = CyaSSL_connect(ssl);

    int error = CyaSSL_get_error(ssl, 0);
    int sockfd = (int)CyaSSL_get_fd(ssl);
    int select_ret;

    while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                                  error == SSL_ERROR_WANT_WRITE)) {
        int currTimeout = 1;

        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        select_ret = tcp_select(sockfd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                                        (select_ret == TEST_ERROR_READY)) {
                    ret = CyaSSL_connect(ssl);
            error = CyaSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS){
        printf("SSL_connect failed\n");
        exit(0);
    }
    return ret;
}
```

We will also need to make some chances to `ClientGreet()` which should now look like this:

```c
/* 
 * clients initial contact with server. (socket to connect, security layer)
 */
int ClientGreet(CYASSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int ret = 0;

    printf("Message for server:\t");
    fgets(sendBuff, MAXDATASIZE, stdin);

    if (CyaSSL_write(ssl, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = CyaSSL_get_error(ssl, 0);
        printf("Write error: Error: %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = CyaSSL_read(ssl, rcvBuff, MAXDATASIZE);   
    if (ret <= 0) {
        /* the server failed to send data, or error trying */
        ret = CyaSSL_get_error(ssl, 0);
        while (ret == SSL_ERROR_WANT_READ) {
            ret = CyaSSL_read(ssl, rcvBuff, MAXDATASIZE);
            ret = CyaSSL_get_error(ssl, 0);
        }
        if (ret < 0) {
            ret = CyaSSL_get_error(ssl, 0);
            printf("Read error. Error: %d\n", ret);
            return EXIT_FAILURE;
        }
    }
    printf("Recieved: \t%s\n", rcvBuff);

    return ret;
}
```

Now we need to check for non-blocking in our `Security()` function. To do this, we change:
```c
CyaSSL_set_fd(ssl, sock);

ret = CyaSSL_connect(ssl);
if (ret == SSL_SUCCESS) {
    ret = ClientGreet(sock, ssl);
}
```
to
```c
CyaSSL_set_fd(ssl, sock);
CyaSSL_set_using_nonblock(ssl, 1);
ret = NonBlockConnect(ssl);
if (ret == SSL_SUCCESS) {
    ret = ClientGreet(ssl);
}
```

Next, we need to change the socket to Non-blocking. This is done by adding the following command after the creation of the socket:

```c
/* sets socket to non-blocking */ 
fcntl(sockfd, F_SETFL, O_NONBLOCK);
```

Then we will need to delay connection to the port until it finishes rewriting itself to a non-blocking state. So we have to change how we connect.  Instead of this:

```c
if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
    /* if socket fails to connect to the server*/
    ret = errno;
    printf("Connect error. Error: %i\n", ret);
    return EXIT_FAILURE;
}
```

we need this:

```c
/* keeps trying to connect to the socket until it is able to do so */
while (ret != 0) 
    ret = connect(sockfd, (struct sockaddr *) &servAddr, 
        sizeof(servAddr)); 
```

This keeps trying to connect to the socket until it stops being busy and allows the connection.

## Starting the TLS CLient & Server

Before running `make` be sure that SERV_PORT is the same on both source .c files (11111 by default).  You must also have the IP address of the machine where the server is going to be on.  

To do this type `ifconfig` in your terminal and look for `inet addr:` under the `wlan` section.  If you are going to be running the client and server off of the same machine look under the `lo` section for the local IP address.  

Run `make` in both the server and client folders, this will compile the source files into an executable file. Anytime you make changes to these source files you will need to re-run `make`. Once this is done you can now start both the client and server. To start the server navigate to the server folder and type `./server-tls` and press enter.  To activate the client navigate to the client folder and type `./client-tls <IP Address>` into your terminal.  This should work on both the secured and unsecured programs.  

Congratulations, you now have a basic client and server that can communicate with each other in an unsecured or secured manner.  

=======
