TCP/TLS Tutorial
================

<<<<<<< HEAD
## Client TLS Tutorial

### Basic TLS Client

Again, we will need to import the security library.  Just like in the server, add an `#include` statement in your client program.  Next we will need to add a global `cert` variable: 

```c
const char* cert = "../certs/ca-cert.pem";
```

Now comes changing the 'ClientGreet()' function so its arguments and functions incorporate the security library.

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

### Adding Session Resumption

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

### Adding Non-blocking I/O

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
>>>>>>> a62664a86549dd488e2fa5ae902aa66da8b7a85d
