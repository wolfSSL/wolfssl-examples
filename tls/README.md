Tutorial
========

This tutorial will teach you how to install and run a basic TCP Server and Client. As well as how to incorporate wolfSSL TLS and some additional features on top of these basic examples. It is expected that you have a basic understanding of a simple tcp server/client. If not, before continueing please take a moment to look over the `server-tcp.c` and `client-tcp.c` file which contains the basic tcp server that we will be expanding upon in this tutorial.


First you will need `gcc` and `make` installed on your terminal. You can do this by opening a new terminal window and typing:

    sudo apt-get install gcc make

## Index
1. [Incorporating wolfSSL TLS](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#incorporating-wolfssl-tls)
  * [Installing wolfSSL](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#installing-wolfssl)
2. [Server TLS Tutorial](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#server-tls-tutorial)
  * [Basic TLS Server](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#basic-tls-server)
  * [Adding Multi-threading](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-server-multi-threading)
  * [Adding Non-blocking I/O](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-server-non-blocking-io)
3. [Client TLS Tutorial](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#client-tls-tutorial)
  * [Basic TLS Client](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#basic-tls-client)
  * [Adding Session Resumption](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-client-session-resumption)
  * [Adding Non-blocking I/O](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#adding-client-non-blocking-io)
4. [Starting the TLS Client & Server](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/tutorial-tcp-tls.md#starting-the-tls-client--server)

## Incorporating wolfSSL TLS
To incorporate wolfSSL TLS into your client or server you need to first configure and install the wolfSSL library to your linux machine. After you have done that, you can then run `make` to compile the TLS versions into an executeable.

### Installing wolfSSL
+ Download and extract the wolfSSL package from [here.](http://wolfssl.com/yaSSL/Products-wolfssl.html)
+ In terminal, navigate to the root of the extracted folder.
+ Type `./configure` press enter. Wait until it finishes configuring.
+ Type `make` press enter. 
+ Type `sudo make install`, this will install the wolfSSL libraries to your machine.

wolfSSL libraries should now be installed to your machine and ready to use. You can now make and run the server and client examples.

## Server TLS Tutorial

### Basic TLS Server
To begin, we will be re-writing the basic `server-tcp.c` with wolfSSL. The structure of the file will be almost identical. To begin, we will need to include the wolfssl libraries with the rest of our includes at the top of the file: 

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>

/* include the wolfSSL library for our TLS 1.2 security */
#include <wolfssl/ssl.h>
```
Next we will add our `#define DEFAULT_PORT 11111` and the prototype for our `AcceptAndRead` function:

```c
#define DEFAULT_PORT 11111

int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in 
    clientAddr);
```
Now we will build our `main()` function for the program. What happens here is we create a WOLFSSL context pointer and a socket. We then initialize wolfSSL so that it can be used. After that we tell wolfSSL where our certificate and private key files are that we want our server to use. We then attach our socket to the `DEFAULT_PORT` that we defined above. The last thing to do in the main function is to listen for a new connection on the socket that we binded to our port above. When we get a new connection, we call the `AcceptAndRead` function. The main function should look like: 

```c
int main()
{
    /* Create a ctx pointer for our ssl */
    WOLFSSL_CTX* ctx;

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

    /* Initialize wolfSSL */
    wolfSSL_Init();

    /* If positive value, the socket is valid */
    if (sockfd == -1) {
        printf("ERROR: failed to create the socket\n");
        return EXIT_FAILURE;        /* Kill the server with exit status 1 */        
    }

    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* Load server certificate into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, "../certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, "../certs/server-key.pem", 
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

    wolfSSL_CTX_free(ctx);   /* Free WOLFSSL_CTX */
    wolfSSL_Cleanup();       /* Free wolfSSL */
    return EXIT_SUCCESS;
}
```
Now all that is left is the `AcceptAndRead` function. This function accepts the new connection and passes it off to its on file descriptor `connd`. We then create our ssl object and direct it to our clients connection. Once thats done we jump into a `for ( ; ; )` loop and do a `wolfSSL_read` which will decrypt and send any data the client sends to our `buff` array. Once that happens we print the data to the console and then send a reply back to the client letting the client know that we reicieved their message. We then break out of the loop, free our ssl and close the `connd` connection since it's no longer used. We then `return 0` which tells our loop in main that it was successful and to continue listening for new connections. 

Here is the `AcceptAndRead` with more detailed comments on what's happening.

```c
int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in 
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
        WOLFSSL*     ssl;

        if ( (ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            exit(EXIT_FAILURE);
        }

        /* direct our ssl to our clients connection */
        wolfSSL_set_fd(ssl, connd);

        printf("Using Non-Blocking I/O: %d\n", wolfSSL_get_using_nonblock(
            ssl));

        for ( ; ; ) {
            char buff[256];
            int  ret = 0;

            /* Clear the buffer memory for anything  possibly left over */
            memset(&buff, 0, sizeof(buff));

            /* Read the client data into our buff array */
            if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
                /* Print any data the client sends to the console */
                printf("Client: %s\n", buff);
                
                /* Reply back to the client */
                if ((ret = wolfSSL_write(ssl, reply, sizeof(reply)-1)) 
                    < 0)
                {
                    printf("wolfSSL_write error = %d\n", wolfSSL_get_error(ssl, ret));
                }
            }
            /* if the client disconnects break the loop */
            else {
                if (ret < 0)
                    printf("wolfSSL_read error = %d\n", wolfSSL_get_error(ssl
                        ,ret));
                else if (ret == 0)
                    printf("The client has closed the connection.\n");

                break;
            }
        }
        wolfSSL_free(ssl);           /* Free the WOLFSSL object */
    }
    close(connd);               /* close the connected socket */

    return 0;
}
```
And with that, you should now have a basic TLS server that accepts a connection, reads in data from the client, sends a reply back, and closes the clients connection. 

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/server-tls.c)**

### Adding Server Multi-threading
To add multi-threading support to the basic `tls-server.c` that we created above, we will be using pthreads. Multi-threading will allow the server to handle multiple client connections at the same time. It will pass each new connection off into it's own thread. To do this we will create a new function called `ThreadHandler`. This function will be passed off to its own thread when a new client connection is accepted. We will also be making some minor changes to our `main()` and `AcceptAndRead` functions.

We will start by adding a `#include <pthread.h>` followed by removing our `WOLFSSL_CTX* ctx` from our main() function and making it global. This will allow our threads to have access to it. Because we are no long passing it into the `AcceptAndRead` function, we need to modify the prototype function to no longer take a `wolfSSL_CTX` parameter. The top of your file should now look like:

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

/* Include the wolfSSL library for our TLS 1.2 security */
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

int  AcceptAndRead(socklen_t sockfd, struct sockaddr_in clientAddr);
void *ThreadHandler(void* socketDesc);

/* Create a ctx pointer for our ssl */
WOLFSSL_CTX* ctx;
```
Moving down to our `main()` function, we need to now remove `WOLFSSL_CTX* ctx` from the top of the function since it is now a global variable. Lastly we need modify the `while` loop at the bottom of `main()` to look like:

```c
  printf("Waiting for a connection...\n");
    /* Continuously accept connects while not currently in an active connection
       or told to quit */
    while (loopExit == 0) {
        /* Listen for a new connection, allow 5 pending connections */
        ret = listen(sockfd, 5);
        if (ret == 0) {

            /* Accept client connections and read from them */
            loopExit = AcceptAndRead(sockfd, clientAddr);
        }
```
As you can tell, when we call 
    loopExit = AcceptAndRead(sockfd, clientAddr);
we are no longer passing in our `WOLFSSL_CTX` pointer since it's now global.

Moving on, we can now modify our `AcceptAndRead` function. This function will now pass accepted connections off into their own thread using `pthreads`. This new thread will then loop, reading and writing to the connected client. Because of this most of this function will be moved into the 'ThreadHandler' function. The `AcceptAndRead` function will now look like:

```c
int AcceptAndRead(socklen_t sockfd, struct sockaddr_in clientAddr)
{
    socklen_t size = sizeof(clientAddr);
    int connd;      /* Identify and access the clients connection */

    pthread_t thread_id;

    /* Wait until a client connects */
    while ((connd = accept(sockfd, (struct sockaddr *)&clientAddr, 
        &size))) {
        /* Pass the client into a new thread */
        if (pthread_create(&thread_id, NULL, ThreadHandler, (void *)
            &connd) < 0) {
            perror("could not create thread");
        }
        printf("Handler assigned\n");
    }
    if (connd < 0) {
        perror("accept failed");
    }

    return 0;
}
```

Now that we have that passing client connections to their own threads, we need to create the `ThreadHandler`, this function will act just like the original `AcceptAndRead` function, just in it's own thread so that we can have multiple clients connected. In this function we will be create our `ssl` object and directing it at our client. It will continuously run in a `for ( ; ; )` loop, reading and writing to the connected client until the client disconnects. It should look like: 

```c
void *ThreadHandler(void* socketDesc)
{
    int     connd = *(int*)socketDesc;
    WOLFSSL* ssl;
    /* Create our reply message */
    const char reply[] = "I hear ya fa shizzle!\n";

    printf("Client connected successfully\n");

    if ( (ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        exit(EXIT_FAILURE);
    }

    /* Direct our ssl to our clients connection */
    wolfSSL_set_fd(ssl, connd);

    for ( ; ; ) {
        char buff[256];
        int  ret = 0;

        /* Clear the buffer memory for anything  possibly left over */
        memset(&buff, 0, sizeof(buff));

        /* Read the client data into our buff array */
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
            /* Print any data the client sends to the console */
            printf("Client on Socket %d: %s\n", connd, buff);
            
            /* Reply back to the client */
            if ((ret = wolfSSL_write(ssl, reply, sizeof(reply)-1)) 
                < 0) {
                printf("wolfSSL_write error = %d\n", wolfSSL_get_error(ssl, ret));
            }
        }
        /* if the client disconnects break the loop */
        else {
            if (ret < 0)
                printf("wolfSSL_read error = %d\n", wolfSSL_get_error(ssl
                    ,ret));
            else if (ret == 0)
                printf("The client has closed the connection.\n");

            wolfSSL_free(ssl);           /* Free the WOLFSSL object */
            close(connd);               /* close the connected socket */
            break;
        }
    }

    exit(EXIT_SUCCESS);
}

```
And that's it. You now have a TLS server using multi-threading to handle multiple clients in seperate threads. 

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/server-tls-threaded.c)**

### Adding Server Non-blocking I/O
A Nonblocking server, like the multi-threaded server, can handle multiple connections. It does this by moving clients to the next available socket and handling them all from a single thread. The benefit to this approach is that it there is much less memory consumption than creating a new thread for each connected client. Nonblocking is also much faster. 100 clients connected through nonblocking would be much faster than 100 clients through multi-threading. However, for non-blocking to work the connecting client must also be configured to work with non-blocking servers. 

To add non-blocking input and ouput to our `server-tls.c` file we will need to create two new functions `TCPSelect` and `NonBlocking_ReadWriteAccept`. We will also create an enum that will be used to tell our `NonBlocking_ReadWriteAccept` what exactly to do, does it need to Read? Does it need to Write? or does it need to Accept? this is done so we can re-use the same code and not end up writing three more functions that practically do that same thing. 

To start, let's create our enumerator and prototype functions. The top of your file should look like: 

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <errno.h>

/* Include the wolfSSL library for our TLS 1.2 security */
#include <wolfssl/ssl.h>

#define DEFAULT_PORT 11111

/* Create an enum that we will use to tell our 
 * NonBlocking_ReadWriteAccept() method what to do
 */
enum read_write_t {WRITE, READ, ACCEPT};

int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t socketfd, 
    struct sockaddr_in clientAddr);
int TCPSelect(socklen_t socketfd);
int NonBlocking_ReadWriteAccept(WOLFSSL* ssl, socklen_t socketfd, 
    enum read_write_t rw);
```

We will start by modifying our `main()` function. Because we are using non-blocking we need to give our socket specific options. To do this, we will make a `setsockopt()` call just above our `wolfSSL_init()` call. It should look something like the following: 

```c
 /* If positive value, the socket is valid */
    if (socketfd == -1) {
        printf("ERROR: failed to create the socket\n");
        exit(EXIT_FAILURE);        /* Kill the server with exit status 1 */
    }
    /* Set the sockets options for use with nonblocking i/o */
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &on, len) 
        < 0)                    
        printf("setsockopt SO_REUSEADDR failed\n");

    /* Initialize wolfSSL */
    wolfSSL_Init();
```

Now we are going to re-write our `AcceptAndRead()` function such that it now takes into consideration non-blocking I/O. This function will be directing our `ssl` object to our client. we will then use a `for ( ; ; )` loop to call our `NonBlocking_ReadWriteAccept()` function passing our `READ` and `WRITE` enums in respectively. This tells our `NonBlocking_ReadWriteAccept()` function what we currently want it to do; read or write.

The function, with more detailed comments should now look like: 

```c
int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t socketfd, struct sockaddr_in clientAddr)
{
    socklen_t     size = sizeof(clientAddr);

    /* Wait until a client connects */
    int connd = accept(socketfd, (struct sockaddr *)&clientAddr, &size);

    /* If fails to connect, loop back up and wait for a new connection */
    if (connd == -1) {
        printf("failed to accept the connection..\n");
    }
    /* If it connects, read in and reply to the client */
    else {
        printf("Client connected successfully!\n");
        WOLFSSL* ssl;
        if ( (ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            exit(EXIT_FAILURE);
        }

        /* Direct our ssl to our clients connection */
        wolfSSL_set_fd(ssl, connd);
        
        /* Sets wolfSSL_accept(ssl) */
        if(NonBlocking_ReadWriteAccept(ssl, socketfd, ACCEPT) < 0)
            return 0;

        /* 
         * loop until the connected client disconnects
         * and read in any messages the client sends
         */
        for ( ; ; ) {   
            /* Read data in when I/O is available */
            if (NonBlocking_ReadWriteAccept(ssl, socketfd, READ) == 0)
                break;
            /* Write data out when I/O is available */
            if (NonBlocking_ReadWriteAccept(ssl, socketfd, WRITE) == 0)
                break;
        }
        wolfSSL_free(ssl);           /* Free the WOLFSSL object */
    } 
    close(connd);               /* close the connected socket */

    return 0;
}
```

Next we will write the `NonBlocking_ReadWriteAccept()` function. This function in short swtiches between doing `wolfSSL_accept()`, `wolfSSL_read()`, and `wolfSSL_write()`. It uses a while loop to loop on the socket and assign new client connections to the next available socket using the `TCPSelect()` function we will be writing soon. It then asks each socket if it wants read or wants write. This function should look like: 

```c
/* Checks if NonBlocking I/O is wanted, if it is wanted it will
 * wait until it's available on the socket before reading or writing */
int NonBlocking_ReadWriteAccept(WOLFSSL* ssl, socklen_t socketfd, 
    enum read_write_t rw)
{
    const char reply[] = "I hear ya fa shizzle!\n";
    char       buff[256];
    int        rwret = 0;
    int        selectRet;
    int        ret;


    /* Clear the buffer memory for anything  possibly left 
       over */
    memset(&buff, 0, sizeof(buff));

    if (rw == READ)
        rwret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
    else if (rw == WRITE)
        rwret = wolfSSL_write(ssl, reply, sizeof(reply)-1);
    else if (rw == ACCEPT)
        rwret = wolfSSL_accept(ssl);

    if (rwret == 0) {
        printf("The client has closed the connection!\n");
        return 0;
    }
    else if (rwret != SSL_SUCCESS) {
        int error = wolfSSL_get_error(ssl, 0);

        /* while I/O is not ready, keep waiting */
        while ((error == SSL_ERROR_WANT_READ || 
            error == SSL_ERROR_WANT_WRITE)) {

            if (error == SSL_ERROR_WANT_READ)
                printf("... server would read block\n");
            else
                printf("... server would write block\n");

            selectRet = TCPSelect(socketfd);

            if ((selectRet == 1) || (selectRet == 2)) {
                if (rw == READ)
                    rwret = wolfSSL_read(ssl, buff, sizeof(buff)-1);
                else if (rw == WRITE)
                    rwret = wolfSSL_write(ssl, reply, sizeof(reply)-1);
                else if (rw == ACCEPT)
                    rwret = wolfSSL_accept(ssl);
                
                error = wolfSSL_get_error(ssl, 0);
            }
            else {
                error = SSL_FATAL_ERROR;
                return -1;
            }
        }
        /* Print any data the client sends to the console */
        if (rw == READ)
            printf("Client: %s\n", buff);
        /* Reply back to the client */
        else if (rw == WRITE) {
            if ((ret = wolfSSL_write(ssl, reply, sizeof(reply)-1)) < 0) {
                printf("wolfSSL_write error = %d\n", 
                    wolfSSL_get_error(ssl, ret));
            }
        }
    }

    return 1;
}
```
Lastly, we just need to write our `TCPSelect()` function which will check whether or not any socket is ready for reading and writing and set it accordingly. It should look like: 

```c
int TCPSelect(socklen_t socketfd)
{
    fd_set recvfds, errfds;
    int nfds = socketfd + 1;
    int result;

    FD_ZERO(&recvfds);
    FD_SET(socketfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(socketfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, NULL);

    if (result > 0) {
        if (FD_ISSET(socketfd, &recvfds))
            return 1; /* RECV READY */
        else if (FD_ISSET(socketfd, &errfds))
            return 2; /* ERROR READY */
    }

    return -1; /* TEST FAILED */

}
```
And now you should have a functional TCP TLS Server that uses Nonblocking input and output to accept multiple connections without the use of multi-threading.

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/server-tls-nonblocking.c)**

## Client TLS Tutorial

### Basic TLS Client

Again, we will need to import the security library.  Just like in the server, add an `#include` statement in your client program.  Next we will need to add a global `cert` variable: 

```c
const char* cert = "../certs/ca-cert.pem";
```

Now comes changing the `ClientGreet()` function so its arguments and functions incorporate the security library.

```c
void ClientGreet(int sock, WOLFSSL* ssl)

if (wolfSSL_write(ssl, send, strlen(send)) != strlen(send)) {

if (wolfSSL_read(ssl, receive, MAXDATASIZE) == 0) {
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
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;    /* create WOLFSSL object */
    int         ret = 0;

    wolfSSL_Init();      /* initialize wolfSSL */

    /* create and initiLize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        printf("SSL_CTX_new error.\n");
        return EXIT_FAILURE;
    }

    /* load CA certificates into wolfSSL_CTX. which will verify the server */
    if (wolfSSL_CTX_load_verify_locations(ctx, cert, 0) != SSL_SUCCESS) {
        printf("Error loading %s. Please check the file.\n", cert);
        return EXIT_FAILURE;
    }
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        return EXIT_FAILURE;
    }
    wolfSSL_set_fd(ssl, sock);

    ret = wolfSSL_connect(ssl);
    if (ret == SSL_SUCCESS) {
        ret = ClientGreet(sock, ssl);
    }

    /* frees all data before client termination */
    wolfSSL_free(ssl);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return ret;
}
```

As you can see, this is where we make the call to “greet” the server.  This function sends its certification, `../ca-certs.pem` to the server which checks for this. If it’s there, it establishes the connection and secures the information being sent and received between the two.  Once this has been done, it frees all the data so no processes remain after the connection has been terminated.

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/client-tls.c)**

### Adding Client Session Resumption

In case the connection to the server gets lost, and you want to save time, you’ll want to be able to resume the connection. In this example, we disconnect from the server, then reconnect to the same session afterwards, bypassing the handshake process and ultimately saving time. To accomplish this, you’ll need to add some variable declarations in `Security()`.

```c
WOLFSSL_SESSION* session = 0;/* wolfssl session */                            
WOLFSSL*         sslResume;  /* create WOLFSSL object for connection loss */
```

Next we'll have to add some code to disconnect and then reconnect to the server. This should be added after your `ClientGreet()` call in `Security()`.

```c
/* saves the session */
session = wolfSSL_get_session(ssl);
wolfSSL_free(ssl);

/* closes the connection */
close(sock);

/* new ssl to reconnect to */
sslResume = wolfSSL_new(ctx);

/* makes a new socket to connect to */
sock = socket(AF_INET, SOCK_STREAM, 0);

/* sets session to old session */
wolfSSL_set_session(sslResume, session);

/* connects to new socket */
if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    /* if socket fails to connect to the server*/
    ret = wolfSSL_get_error(ssl, 0);
    printf("Connect error. Error: %i\n", ret);
    return EXIT_FAILURE;
}

/* sets new file discriptior */
wolfSSL_set_fd(sslResume, sock);

/* reconects to wolfSSL */
ret = wolfSSL_connect(sslResume);
if (ret != SSL_SUCCESS) {
    return ret;
}

/* checks to see if the new session is the same as the old session */
if (wolfSSL_session_reused(sslResume))
    printf("Re-used session ID\n"); 
else
    printf("Did not re-use session ID\n");

/* regreet the client */
ret = ClientGreet(sock, sslResume);
```

We will aslo have to slightly alter our last `wolfSSL_free()` call. Instead of `wolfSSL_free(ssl);` it needs to state `wolfSSL_free(sslResume);`

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/client-tls-resume.c)**

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
int NonBlockConnect(WOLFSSL* ssl)
{
    int ret = wolfSSL_connect(ssl);

    int error = wolfSSL_get_error(ssl, 0);
    int sockfd = (int)wolfSSL_get_fd(ssl);
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
                    ret = wolfSSL_connect(ssl);
            error = wolfSSL_get_error(ssl, 0);
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
int ClientGreet(WOLFSSL* ssl)
{
    /* data to send to the server, data recieved from the server */
    char sendBuff[MAXDATASIZE], rcvBuff[MAXDATASIZE] = {0};
    int ret = 0;

    printf("Message for server:\t");
    fgets(sendBuff, MAXDATASIZE, stdin);

    if (wolfSSL_write(ssl, sendBuff, strlen(sendBuff)) != strlen(sendBuff)) {
        /* the message is not able to send, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        printf("Write error: Error: %d\n", ret);
        return EXIT_FAILURE;
    }

    ret = wolfSSL_read(ssl, rcvBuff, MAXDATASIZE);   
    if (ret <= 0) {
        /* the server failed to send data, or error trying */
        ret = wolfSSL_get_error(ssl, 0);
        while (ret == SSL_ERROR_WANT_READ) {
            ret = wolfSSL_read(ssl, rcvBuff, MAXDATASIZE);
            ret = wolfSSL_get_error(ssl, 0);
        }
        if (ret < 0) {
            ret = wolfSSL_get_error(ssl, 0);
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
wolfSSL_set_fd(ssl, sock);

ret = wolfSSL_connect(ssl);
if (ret == SSL_SUCCESS) {
    ret = ClientGreet(sock, ssl);
}
```
to
```c
wolfSSL_set_fd(ssl, sock);
wolfSSL_set_using_nonblock(ssl, 1);
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

**The finished source code for this can be [found here.](https://github.com/wolfSSL/wolfssl-examples/blob/master/tls/client-tls-nonblocking.c)**

## Starting the TLS CLient & Server

Before running `make` be sure that SERV_PORT is the same on both source .c files (11111 by default).  You must also have the IP address of the machine where the server is going to be on.  

To do this type `ifconfig` in your terminal and look for `inet addr:` under the `wlan` section.  If you are going to be running the client and server off of the same machine look under the `lo` section for the local IP address.  

Run `make` in both the server and client folders, this will compile the source files into an executable file. Anytime you make changes to these source files you will need to re-run `make`. Once this is done you can now start both the client and server. To start the server navigate to the server folder and type `./server-tls` and press enter.  To activate the client navigate to the client folder and type `./client-tls <IP Address>` into your terminal.  This should work on both the secured and unsecured programs.  

Congratulations, you now have a basic client and server that can communicate with each other in an unsecured or secured manner.  

=======
