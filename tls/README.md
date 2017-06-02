wolfSSL SSL/TLS Examples
========================

Here are examples focused on TCP/IP connections. The simplest of which are the
`*-tcp.c` files, which demonstrate a simple client/server TCP/IP connection
without encryption. From there, the `*-tls.c` files demonstrate the same
connection, but modified to utilize wolfSSL to establish a TLS 1.2 connection.

In general, the naming convention of these files mean that if a file is named
in the form `X-Y.c`, then it's a copy of `X.c` intended to demonstrate Y. The
exceptions being `server-tls.c` and `client-tls.c`, as noted above.
Furthermore, the files is formated such that using a diff tool such as
`vimdiff` to compare `X-Y.c` to `X.c` should highlight only the relevant
changes required to convert `X.c` into `X-Y.c`

The files in this directory are presented to you in hopes that they are useful,
especially as a basic starting point. It is fully recognized that these
examples are neither the most sophisticated nor robust. Instead, these examples
are intended to be easy to follow and clear in demonstrating the basic
procedure. It cannot be guaranteed that these programs will be free of memory
leaks, especially in error conditions.

Tutorial
========

This portion of the `README` is dedicated to walking you through creating most
of the files in this directory.

Before we begin, there are a few important things to note.

First, code will be referred to in terms of "blocks". Blocks are named by the
comment at the top of them. Whenever we write code, we should also write the
comments so that we can identify the blocks. For example, the following code
contains two blocks:

```c
    /* Hello, world! */
    printf("Hello, world!\n");

    /* check ret */
    ret = foo()
    if (ret == -1) {
        printf("foo() returned -1\n");
    }
```

When we refer to the "Hello, world!" block, we should look at the `printf()`
statement. Similarly, when we refer to the "check ret" block, we should look at
everything from the comment to the bottom of the `if` statement.

Second, all references to files are made relative to this `tls/` directory. Any
file reference should then be modified to point to the correct location.

Third, it is recommended that you make a new directory to write your files
into.



## Table of contents

1. [Running these examples](#run)
2. [Compiling these examples](#compile)
3. [A simple TCP client/server pair](#tcp)

    1. [Server](#server-tcp)
    2. [Client](#client-tcp)
    3. [Running](#run-tcp)

4. [Converting to use wolfSSL](#tls)

    1. [Server](#server-tls)
    2. [Client](#client-tls)
    3. [Running](#run-tls)

4. [Using callbacks](#callback)

    1. [Running](#run-callback)

4. [Using ECC](#ecc)

    1. [Server](#server-ecc)
    2. [Client](#client-ecc)
    3. [Running](#run-ecc)



## <a name="run">Running these examples</a>

If the client is running on the same machine as the server, use 127.0.0.1 as
the IPv4 address when calling the client.

Otherwise, if the server is on a remote machine, learn the IPv4 address of that
machine and use that instead.

If you have a tool such as Wireshark, you can use it to inspect the connection.

If you have any troubles with compiling or running that do not seem to be due
to the code, see [the section on compiling](#compile) below.



## <a name="compile">Compiling these examples</a>

If you stick to the naming convention, copying the makefile `Makefile` (which
can be found [here][make]) into the directory where you are working should
allow you to simply use `make` to compile.

The makefile is written such that it knows when it is appropriate to include
pthread support and if it is necessary to link against wolfSSL. You also have
access to `make clean` to remove all of the compiled programs.

A simple call to `make` is equivalent to `make all`, meaning any file `*.c`
will be compiled. If this is not desired, calling `make X` will only compile
`X.c`.

Furthermore, for the TLS examples, you are assumed to have installed wolfSSL in
`/usr/local/`. The makefile will take care of pointing to `/usr/local/include/`
to find the wolfSSL headers and `/usr/local/lib/` to link against the wolfSSL
library. This linking is dynamic, however, so `/usr/local/lib/` must be in your
linker's path. If you are receiving errors at runtime about loading shared
libraries, run this command from the terminals you are running the TLS examples
from:

```bash
export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):/usr/local/lib
```

If you have installed wolfSSL in another location, edit `Makefile` so that the
`LIB_PATH` variable reflects this.

If you have configured wolfSSL to use static linking, comment out the line
```makefile
LIBS+=$(DYN_LIB)
```
and uncomment the line
```makefile
#LIBS+=$(STATIC_LIB)
```
to statically link against wolfSSL in these examples.

For `client-tls-writedup`, it is required that wolfSSL be configured with the
`--enable-writedup` flag. Remember to build and install wolfSSL after
configuring it with this flag.



## <a name="tcp">A simple TCP client/server pair</a>

We'll begin by making a simple client/server pair that will communicate in
plaintext over a TCP socket.

#### <a name="server-tcp">Server</a>

We'll write the server first. We'll be rewriting `server-tcp.c` from scratch,
so there's no reason to copy anything. The finished version can be found
[here][s-tcp].

To start, let's write up a quick skeleton for this file like this:

```c
/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define DEFAULT_PORT 11111



int main()
{
    int                sockfd;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;



    /* We'll fill in our work here */



    /* Cleanup and return */
    return 0;               /* Return reporting a success               */
}
```
<<<<<<< HEAD

Let's go over all that.

"The usual suspects" are some of the usual includes you'd expect to see in a C
file. Beyond them, the "socket includes" are all of the includes we need to do
socket operations. `sys/sockets` gives us our standard definitions for sockets,
and `arpa/inet` and `netinet/in.h` each give us a few functions for dealing
with internet sockets.

`DEFAULT_PORT` is a quick definition for which port to bind to.

There are quite a few variables here. We'll talk about them a bit more in depth
when we get to using them. As a quick description, however, `sockfd` and
`connd` are file descriptors; `servAddr`, `clientAddr`, and `size` are used to
describe the address of the server and client; `buff` and `len` are for I/O;
and finally `shutdown` is for flow control.

Now we'll set up the sockets.

The next step is to get ahold of a socket for our server. Replace the "We'll
fill in our work here" comment with these lines:
=======
Next we will add our `#define DEFAULT_PORT 11111` and the prototype for our
`AcceptAndRead` function:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }
```

The comment quickly explains the arguments passed to `socket()`. Otherwise,
`socket()` returns a -1 on error.

This is also an opportunity to introduce the error convention for these
examples: print an error message and end execution. It's important to realise
that this is not a graceful way to do this. In the event of an error, we'll
often end up ending execution with quite a bit of memory allocated. For the
purpose of these examples, however, it was decided that dealing with these
errors gracefully would obscure the purposes of the examples. As such, we make
a nod at the importance of error handling without complicating the matter by
putting in the full amount of effort.

<<<<<<< HEAD
Anyway, now that we've opened a socket, we should update the "Cleanup and
return" section at the bottom of `main()` to close it when we're done. It
should look a bit like this now:

```c
    /* Cleanup and return */
    close(sockfd);          /* Close the socket listening for clients   */
    return 0;               /* Return reporting a success               */
```

Now that we have a socket, let's fill out our address. Just after the "Create a
socket [...]" block, add these lines:
=======
int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in
    clientAddr);
```
Now we will build our `main()` function for the program. What happens here is
we create a WOLFSSL context pointer and a socket. We then initialize wolfSSL so
that it can be used. After that we tell wolfSSL where our certificate and
private key files are that we want our server to use. We then attach our socket
to the `DEFAULT_PORT` that we defined above. The last thing to do in the main
function is to listen for a new connection on the socket that we binded to our
port above. When we get a new connection, we call the `AcceptAndRead` function.
The main function should look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

<<<<<<< HEAD
    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */
```
=======
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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

That "Initialize the sever address sturuct wit zeros" step is not strictly
necessary, but it's usually a good idea, and it doesn't complicate the example
too much.

<<<<<<< HEAD
With the address all filled out, the final stage of setting up the server is to
bind our socket to a port and start listening for connections. In total, this
should look like this:
=======
    /* If positive value, the socket is valid */
    if (sockfd == -1) {
        printf("ERROR: failed to create the socket\n");
        return EXIT_FAILURE;        /* Kill the server with exit status 1 */
    }
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }

<<<<<<< HEAD
    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
=======
    /* Load server certificate into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, "../certs/server-cert.pem",
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a
    }
```

<<<<<<< HEAD
`bind()` makes it such that our server on `sockfd` is now visible at the
location described by `servAddr`, and `listen()` causes us to start listening
to `sockfd` for incoming client connections.

And now the setup is complete. Now we just need to deal with I/O.
=======
    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, "../certs/server-key.pem",
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-key.pem, please check"
                "the file.\n");
        return EXIT_FAILURE;
    }

    /* Initialize the server address struct to zero */
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

We're going to keep accepting clients until one of them tells us "shutdown". To
start, let's write up a quick skeleton for this part of the code:

```c
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept clients here */

        printf("Client connected successfully\n");



        /* Do communication here */



        /* Cleanup after this connection */
    }

    printf("Shutdown complete\n");
```
<<<<<<< HEAD
=======
Now all that is left is the `AcceptAndRead` function. This function accepts the
new connection and passes it off to its on file descriptor `connd`. We then
create our ssl object and direct it to our clients connection. Once thats done
we jump into a `for ( ; ; )` loop and do a `wolfSSL_read` which will decrypt
and send any data the client sends to our `buff` array. Once that happens we
print the data to the console and then send a reply back to the client letting
the client know that we reicieved their message. We then break out of the loop,
free our ssl and close the `connd` connection since it's no longer used. We
then `return 0` which tells our loop in main that it was successful and to
continue listening for new connections.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

We'll deal with accepting clients first. Replace the "Accept clients here"
comment with these lines:

```c
<<<<<<< HEAD
        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            return -1;
        }
```
=======
int AcceptAndRead(WOLFSSL_CTX* ctx, socklen_t sockfd, struct sockaddr_in
    clientAddr)
{
        /* Create our reply message */
    const char reply[]  = "I hear ya fa shizzle!\n";
    socklen_t         size    = sizeof(clientAddr);
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

This call will block execution until a client connects to our server. At which
point, we'll get a connection to the client through `connd`. Now that we've
opened a new file, we should close it when we're done with it. Update the
"Cleanup after this connection" section at the bottom of the loop to close
`connd`. It should look a bit like this now:

```c
        /* Cleanup after this connection */
        close(connd);           /* Close the connection to the client   */
```

Now that we have a connection to the client, we can do communication.

First, we'll read from the client. Replace the "Do communication here" comment
with these lines:

```c
        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if (read(connd, buff, sizeof(buff)-1) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            return -1;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);
```

<<<<<<< HEAD
This zeros out the buffer, then gets a message from the client and prints it
to `stdout` so we can see it. Recall that we want to shutdown when the client
tells us "shutdown". To accomplish this, add these lines after we print the
message:
=======
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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }
```

Note that this block doesn't end execution or break out of the loop right away.
We still want to respond to the client with our own message.

After reading the message from the client, we first write our message into the
buffer like this:

```c
        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, "I hear ya fa shizzle!\n", sizeof(buff));
        len = strnlen(buff, sizeof(buff));
```

And then we send it to the client like this:

```c
        /* Reply back to the client */
        if (write(connd, buff, len) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }
```
<<<<<<< HEAD
=======
And with that, you should now have a basic TLS server that accepts a
connection, reads in data from the client, sends a reply back, and closes the
clients connection.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

And we're done.

We've set up a server on a TCP socket and dealt with a quick back-and-forth
with a client.

<<<<<<< HEAD
Now we just need a client.

#### <a name="client-tcp">Client</a>

Now we'll write the client. We'll be rewriting `client-tcp.c` from scratch, so
there's no reason to copy anything. The finished version can be found
[here][c-tcp].

Once more, we'll start with a skeleton:
=======
### Adding Server Multi-threading
To add multi-threading support to the basic `tls-server.c` that we created
above, we will be using pthreads. Multi-threading will allow the server to
handle multiple client connections at the same time. It will pass each new
connection off into it's own thread. To do this we will create a new function
called `ThreadHandler`. This function will be passed off to its own thread when
a new client connection is accepted. We will also be making some minor changes
to our `main()` and `AcceptAndRead` functions.

We will start by adding a `#include <pthread.h>` followed by removing our
`WOLFSSL_CTX* ctx` from our main() function and making it global. This will
allow our threads to have access to it. Because we are no long passing it into
the `AcceptAndRead` function, we need to modify the prototype function to no
longer take a `wolfSSL_CTX` parameter. The top of your file should now look
like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* socket includes */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define DEFAULT_PORT 11111


<<<<<<< HEAD
=======
/* Create a ctx pointer for our ssl */
WOLFSSL_CTX* ctx;
```
Moving down to our `main()` function, we need to now remove `WOLFSSL_CTX* ctx`
from the top of the function since it is now a global variable. Lastly we need
modify the `while` loop at the bottom of `main()` to look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

int main(int argc, char** argv)
{
    int                sockfd;
    struct sockaddr_in servAddr;
    char               buff[256];
    size_t             len;



    /* We'll fill in our work here */



    /* Cleanup and return */
    return 0;               /* Return reporting a success               */
}
```
<<<<<<< HEAD

It looks quite similar to the server's skeleton, but as you can see, there are
way fewer variables. Similarly, we're taking in command line arguments this
time rather than ignoring them.

This'll be our first step: verify that the program has been called correctly.
There are more sophisticated ways of doing this, but we'll use a simple
solution. Replace the "We'll fill in our work here" comment with the following
lines:

```c
    /* Check for proper calling convention */
    if (argc != 2) {
        printf("usage: %s <IPv4 address>\n", argv[0]);
        return 0;
=======
As you can tell, when we call
```c
loopExit = AcceptAndRead(sockfd, clientAddr);
```
we are no longer passing in our `WOLFSSL_CTX` pointer since it's now global.

Moving on, we can now modify our `AcceptAndRead` function. This function will
now pass accepted connections off into their own thread using `pthreads`. This
new thread will then loop, reading and writing to the connected client. Because
of this most of this function will be moved into the 'ThreadHandler' function.
The `AcceptAndRead` function will now look like:

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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a
    }
```

We expect exactly two arguments: the program's name and an IPv4 address. We
won't be confirming that the second argument is a well formed IPv4 address,
though. If it's not, we'll error out midway through.

Now that we know the program has been called more-or-less correctly, we'll open
a socket to connect to the server through.

```c
    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        return -1;
    }
```

This is just like the call we make in the server code. Similarly, we should
remember to close the socket at the bottom of `main()`. Make the "Cleanup and
return" section look a bit like this:

```c
    /* Cleanup and return */
    close(sockfd);          /* Close the connection to the server       */
    return 0;               /* Return reporting a success               */
```

<<<<<<< HEAD
Now we can fill out the address of the server we want to connect to. After the
"Create a socket [...]" block, add these lines:
=======
Now that we have that passing client connections to their own threads, we need
to create the `ThreadHandler`, this function will act just like the original
`AcceptAndRead` function, just in it's own thread so that we can have multiple
clients connected. In this function we will be create our `ssl` object and
directing it at our client. It will continuously run in a `for ( ; ; )` loop,
reading and writing to the connected client until the client disconnects. It
should look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET;             /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */

    /* Get the server IPv4 address from the command line call */
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) != 1) {
        fprintf(stderr, "ERROR: invalid address\n");
        return -1;
    }
```

Once more, this is quite similar to server code. This time, hovewer, rather
than setting `servAddr.sin_addr.s_addr` to `INADDR_ANY`, we're going to make a
call to `inet_pton()` to read `argv[1]` as an IPv4 address and assign it to the
right place in `servAddr`. If `argv[1]` is well formed, `inet_pton()` will
return a 1. Otherwise, we have an error.

Now we can connect to the server. Add these lines next:

```c
    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr))
        == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        return -1;
    }
```

<<<<<<< HEAD
`connect()` will block until the connection is successful. If something goes
wrong, it returns a -1 that we'll catch as a fatal error.

After that, we're done with setup. We can now have our back-and-forth with the
server. Just to be fun, let's quickly get a message from `stdin` like this:

```c
    /* Get a message for the server from stdin */
    printf("Message for server: ");
    memset(buff, 0, sizeof(buff));
    fgets(buff, sizeof(buff), stdin);
    len = strnlen(buff, sizeof(buff));
```

We know that our server is expecting us to say something first, so let's send
the message like this:

```c
    /* Send the message to the server */
    if (write(sockfd, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
=======
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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a
    }
```

We also know that our server will then send us a reply. Let's write that down
and print it to `stdout` so we can see it:

```c
    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (read(sockfd, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }

    /* Print to stdout any data the server sends */
    printf("Server: %s\n", buff);
```
<<<<<<< HEAD
=======
And that's it. You now have a TLS server using multi-threading to handle
multiple clients in seperate threads.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

And that's everything! Our client will just be a quick one-and-done thing.

<<<<<<< HEAD
#### <a name="run-tcp">Running</a>

`server-tcp` can be connected to by the following:

* `client-tcp`
=======
### Adding Server Non-blocking I/O
A Nonblocking server, like the multi-threaded server, can handle multiple
connections. It does this by moving clients to the next available socket and
handling them all from a single thread. The benefit to this approach is that it
there is much less memory consumption than creating a new thread for each
connected client. Nonblocking is also much faster. 100 clients connected
through nonblocking would be much faster than 100 clients through
multi-threading. However, for non-blocking to work the connecting client must
also be configured to work with non-blocking servers.

To add non-blocking input and ouput to our `server-tls.c` file we will need to
create two new functions `TCPSelect` and `NonBlocking_ReadWriteAccept`. We will
also create an enum that will be used to tell our `NonBlocking_ReadWriteAccept`
what exactly to do, does it need to Read? Does it need to Write? or does it
need to Accept? this is done so we can re-use the same code and not end up
writing three more functions that practically do that same thing.

To start, let's create our enumerator and prototype functions. The top of your
file should look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

`client-tcp` can connect to the following:

* `server-tcp`


<<<<<<< HEAD

## <a name="tls">Converting to use wolfSSL</a>

Now that we have a TCP client/server pair, we can modify them to use TLS 1.2 to
secure their communication.

#### <a name="server-tls">Server</a>

We'll modify the server first. Copy `server-tcp.c` to a new file,
`server-tls.c`, that we will modify. The finished version can be found
[here][s-tls].

The first thing to do is include the wolfSSL header. Just below the "socket
includes" block, add these lines:

```c
/* wolfSSL */
#include <wolfssl/ssl.h>
```

We're also going to need a few more defines for our file paths. Just below the
definition of `DEFAULT_PORT` add these lines:

```c
#define CERT_FILE "../certs/server-cert.pem"
#define KEY_FILE  "../certs/server-key.pem"
```

Before continuing on, make sure that these are the correct paths to those
files.

Now, inside of main, we need two more variables. At the top of `main()`, add
these lines after the other variable declarations:

```c
    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;
```

And now we can start hooking up wolfSSL. The first thing to do is initialize
the wolfSSL library. Beneath the variable declarations add the lines:
=======
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

We will start by modifying our `main()` function. Because we are using
non-blocking we need to give our socket specific options. To do this, we will
make a `setsockopt()` call just above our `wolfSSL_init()` call. It should look
something like the following:

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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Initialize wolfSSL */
    wolfSSL_Init();
```

<<<<<<< HEAD
Next we'll set up the wolfSSL context. After the "Create a socket [...]" block
these lines:

```c
    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }
```

The call to `wolfTLSv1_2_server_method()` inside the call to
`wolfSSL_CTX_new()` is what makes us use the TLS 1.2 protocol.

Just like we have to close `sockfd` when we're done, we'll have to cleanup
wolfSSL. Edit the "Cleanup and return" section at the bottom of `main()` to
look something like this:
=======
Now we are going to re-write our `AcceptAndRead()` function such that it now
takes into consideration non-blocking I/O. This function will be directing our
`ssl` object to our client. we will then use a `for ( ; ; )` loop to call our
`NonBlocking_ReadWriteAccept()` function passing our `READ` and `WRITE` enums
in respectively. This tells our `NonBlocking_ReadWriteAccept()` function what
we currently want it to do; read or write.

The function, with more detailed comments should now look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Cleanup and return */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the socket listening for clients   */
    return 0;               /* Return reporting a success               */
```

Though we're not done with `ctx` yet: we have to load in our certificates.
After the "Create and initialize `WOLFSSL_CTX`" block, add these lines:

```c
    /* Load server certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }
```

<<<<<<< HEAD
Recall that `CERT_FILE` is one of our own defines.
=======
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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

What the above does is load the certificate file `CERT_FILE` so that the server
can verify itself to clients. This certificate is known to be in the PEM format
(indicated by `SSL_FILETYPE_PEM`), and this information is then kept in `ctx`.

<<<<<<< HEAD
Next we need the server to load its private key:

```c
    /* Load server key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        return -1;
    }
```

Recall that `KEY_FILE` is one of our own defines.

Similar to the call to `wolfSSL_CTX_use_certificate_file()` previously, this
loads a private key from `KEY_FILE` of the format `SSL_FILETYPE_PEM`, and
stores the information into `ctx`.

And that's everything for the setup phase. Now we just need to add a few things
to how we handle clients.

After we accept a new client, we need to make a wolfSSL object. Just after the
"Accept client connections" block, add these lines:

```c
        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            return -1;
=======
Next we will write the `NonBlocking_ReadWriteAccept()` function. This function
in short swtiches between doing `wolfSSL_accept()`, `wolfSSL_read()`, and
`wolfSSL_write()`. It uses a while loop to loop on the socket and assign new
client connections to the next available socket using the `TCPSelect()`
function we will be writing soon. It then asks each socket if it wants read or
wants write. This function should look like:

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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);
```
<<<<<<< HEAD
=======
Lastly, we just need to write our `TCPSelect()` function which will check
whether or not any socket is ready for reading and writing and set it
accordingly. It should look like:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

This allocates a new wolfSSL object. It will inherit all of the files we
registered to `ctx`. After that, we give `ssl` the file descriptor `connd` to
hold onto.

We can't forget to free this object either. Update the "Cleanup after this
connection" section at the bottom of the loop to look like this:

```c
        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        close(connd);           /* Close the connection to the client   */
```
<<<<<<< HEAD
=======
And now you should have a functional TCP TLS Server that uses Nonblocking input
and output to accept multiple connections without the use of multi-threading.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

From here on, `ssl` is our new `connd` for purposes of communicating with
clients.

First, this means that we need to replace calls to `read()` with calls to
`wolfSSL_read()` and replace argument `connd` with `ssl`. The "Read the client
data [...]" block now should look like this:

```c
        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
            fprintf(stderr, "ERROR: failed to read\n");
            return -1;
        }
```

<<<<<<< HEAD
Second, we need to do something similar to our calls to `write()`. The "Reply
back to the client" block now should look like this:
=======
Again, we will need to import the security library.  Just like in the server,
add an `#include` statement in your client program.  Next we will need to add a
global `cert` variable:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
        /* Reply back to the client */
        if (wolfSSL_write(ssl, buff, len) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }
```

<<<<<<< HEAD
And after this, we've successfully modified our TCP server to use wolfSSL for
TLS 1.2 connections. Now we just need a client to connect with.
=======
Now comes changing the `ClientGreet()` function so its arguments and functions
incorporate the security library.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

#### <a name="client-tls">Client</a>

Now we'll write the client. Copy `client-tcp.c` to a new file, `client-tls.c`,
that we will modify. The finished version can be found [here][c-tls].

Like the server, the first thing to do is include the wolfSSL header. Just
below the "socket includes" block, add these lines:

```c
/* wolfSSL */
#include <wolfssl/ssl.h>
```

<<<<<<< HEAD
We're going to need a different definition for `CERT_FILE`, however. Just below
the definition of `DEFAULT_PORT` add this line:
=======
You can think of this as, instead of just a normal read and write, it is now a
“secure” read and write.  We also need to change the call to `ClientGreet()` in
`main()`.  Instead of calling directly to it, we should make a call to a
`Security()` that will then check the server for the correct `certs`.  To do
this, change:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
#define CERT_FILE "../certs/ca-cert.pem"
```

Before continuing on, make sure that this is the correct path to that file.

Now, inside of main, we need two more variables. At the top of `main()`, add
these lines after the other variable declarations:

```c
    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;
```

<<<<<<< HEAD
And now we can start hooking up wolfSSL. The first thing to do is initialize
the wolfSSL library. This time we'll do it after we check our calling
convention. Beneath the "Check for proper calling convention" block, add the
lines:

```c
    /* Initialize wolfSSL */
    wolfSSL_Init();
```
=======
Now we just have to make the `Security()` function. It should look something
like:

```c
/*
 * applies TLS 1.2 security layer to data being sent.
 */
int Security(int sock)
{
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;    /* create WOLFSSL object */
    int         ret = 0;
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

Next we set up `ctx`. After the "Create a socket [...]" block, add these
lines:

```c
    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        return -1;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_load_verify_locations(ctx, CERT_FILE, NULL)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                CERT_FILE);
        return -1;
    }
```

That "Create and initialize `WOLFSSL_CTX`" block should be familiar from the
server code.

The "Load client certificates into `WOLFSSL_CTX`" block is how we load
`CERT_FILE` into `ctx` so that we can verify the certificate of the servers we
connect to. It should be noted that all files are assumed to be in the PEM
format.

Before continuing, we should remember to update our "Cleanup and return" block
at the bottom of `main()` to free `ctx` and cleanup wolfSSL. It should now look
like this:

```c
    /* Cleanup and return */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the connection to the server       */
    return 0;               /* Return reporting a success               */
```

<<<<<<< HEAD
And this concludes the setup. Now we just need to deal with the connection to
the server. After the "Connect to the server" block, add these lines:
=======
As you can see, this is where we make the call to “greet” the server.  This
function sends its certification, `../ca-certs.pem` to the server which checks
for this. If it’s there, it establishes the connection and secures the
information being sent and received between the two.  Once this has been done,
it frees all the data so no processes remain after the connection has been
terminated.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        return -1;
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, sockfd);

<<<<<<< HEAD
    /* Connect to wolfSSL on the server side */
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        return -1;
    }
```

Those first two blocks should be familiar from the server code: we're making an
SSL object from the context and giving it our connection to the server.

The third block, "Connect to wolfSSL on the server side", is new. It is
technically optional; If we don't call it, the first time we try to read or
write though `ssl` it will be invoked anyway. We're going to call it ourself to
make things easier when modifying this file to add different features in the
future.

Before continuing, we should remember to update our "Cleanup and return" block
at the bottom of `main()` to free `ssl`. It should now look like this:

```c
    /* Cleanup and return */
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
    close(sockfd);          /* Close the connection to the server       */
    return 0;               /* Return reporting a success               */
```

Now all we have to do is update our `write()` and `read()` calls to use
wolfSSL. The "Send the message to the server" block should now look like this:
=======
In case the connection to the server gets lost, and you want to save time,
you’ll want to be able to resume the connection. In this example, we disconnect
from the server, then reconnect to the same session afterwards, bypassing the
handshake process and ultimately saving time. To accomplish this, you’ll need
to add some variable declarations in `Security()`.

```c
WOLFSSL_SESSION* session = 0;/* wolfssl session */
WOLFSSL*         sslResume;  /* create WOLFSSL object for connection loss */
```

Next we'll have to add some code to disconnect and then reconnect to the
server. This should be added after your `ClientGreet()` call in `Security()`.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
    /* Send the message to the server */
    if (wolfSSL_write(ssl, buff, len) != len) {
        fprintf(stderr, "ERROR: failed to write\n");
        return -1;
    }
```

And the "Read the server data [...]" block should look like this:

```c
    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if (wolfSSL_read(ssl, buff, sizeof(buff)-1) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        return -1;
    }
```

And we're done. We should now have a fully functional TLS client.

#### <a name="run-tls">Running</a>

`server-tls` can be connected to by the following:

* `client-tls`
* `client-tls-callback`
* `client-tls-nonblocking`
* `client-tls-resume`
* `client-tls-writedup`

`client-tls` can connect to the following:

<<<<<<< HEAD
* `server-tls`
* `server-tls-callback`
* `server-tls-nonblocking`
* `server-tls-threaded`
=======
/* checks to see if the new session is the same as the old session */
if (wolfSSL_session_reused(sslResume))
    printf("Re-used session ID\n");
else
    printf("Did not re-use session ID\n");
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a


<<<<<<< HEAD
=======
We will aslo have to slightly alter our last `wolfSSL_free()` call. Instead of
`wolfSSL_free(ssl);` it needs to state `wolfSSL_free(sslResume);`
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

## <a name="callback">Using callbacks</a>

The edits required to make wolfSSL use custom functions for getting I/O from a
socket are identical between the client and server code. As such, pick
whichever `*-tls.c` you want and copy it to `*-tls-callback.c`. The finished
version of the server can be found [here][s-tls-c], and the finished version of
the client can be found [here][c-tls-c].

<<<<<<< HEAD
The first step is to add `errno.h` to the list of "usual suspects". This block
should now look like this:
=======
The first thing that has to be done in order to make a socket a non-blocking
socket is to add another library to the top of the code.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
```

<<<<<<< HEAD
Next, we're going to need write our callbacks functions. These functions will
be called by wolfSSL whenever it need to read or write data through the
sockets. These callbacks can be quite sophisticated, but we're going to write
simple callbacks that tell us how many bytes they read or write.

First, the read callback:
=======
Then we will need to add a few functions and an `enum` after the `cert`
variable.

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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
int my_IORecv(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    /* By default, ctx will be a pointer to the file descriptor to read from.
     * This can be changed by calling wolfSSL_SetIOReadCtx(). */
    int sockfd = *(int*)ctx;
    int recvd;


    /* Receive message from socket */
    if ((recvd = recv(sockfd, buff, sz, 0)) == -1) {
        /* error encountered. Be responsible and report it in wolfSSL terms */

        fprintf(stderr, "IO RECEIVE ERROR: ");
        switch (errno) {
        #if EAGAIN != EWOULDBLOCK
        case EAGAIN: /* EAGAIN == EWOULDBLOCK on some systems, but not others */
        #endif
        case EWOULDBLOCK:
            if (!wolfSSL_dtls(ssl) || wolfSSL_get_using_nonblock(ssl)) {
                fprintf(stderr, "would block\n");
                return WOLFSSL_CBIO_ERR_WANT_READ;
            }
            else {
                fprintf(stderr, "socket timeout\n");
                return WOLFSSL_CBIO_ERR_TIMEOUT;
            }
        case ECONNRESET:
            fprintf(stderr, "connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        case EINTR:
            fprintf(stderr, "socket interrupted\n");
            return WOLFSSL_CBIO_ERR_ISR;
        case ECONNREFUSED:
            fprintf(stderr, "connection refused\n");
            return WOLFSSL_CBIO_ERR_WANT_READ;
        case ECONNABORTED:
            fprintf(stderr, "connection aborted\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        default:
            fprintf(stderr, "general error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    else if (recvd == 0) {
        printf("Connection closed\n");
        return WOLFSSL_CBIO_ERR_CONN_CLOSE;
    }

    /* successful receive */
    printf("my_IORecv: received %d bytes from %d\n", sz, sockfd);
    return recvd;
}
```

Next the write callback:

```c
int my_IOSend(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
    /* By default, ctx will be a pointer to the file descriptor to write to.
     * This can be changed by calling wolfSSL_SetIOWriteCtx(). */
    int sockfd = *(int*)ctx;
    int sent;


    /* Receive message from socket */
    if ((sent = send(sockfd, buff, sz, 0)) == -1) {
        /* error encountered. Be responsible and report it in wolfSSL terms */

        fprintf(stderr, "IO SEND ERROR: ");
        switch (errno) {
        #if EAGAIN != EWOULDBLOCK
        case EAGAIN: /* EAGAIN == EWOULDBLOCK on some systems, but not others */
        #endif
        case EWOULDBLOCK:
            fprintf(stderr, "would block\n");
            return WOLFSSL_CBIO_ERR_WANT_READ;
        case ECONNRESET:
            fprintf(stderr, "connection reset\n");
            return WOLFSSL_CBIO_ERR_CONN_RST;
        case EINTR:
            fprintf(stderr, "socket interrupted\n");
            return WOLFSSL_CBIO_ERR_ISR;
        case EPIPE:
            fprintf(stderr, "socket EPIPE\n");
            return WOLFSSL_CBIO_ERR_CONN_CLOSE;
        default:
            fprintf(stderr, "general error\n");
            return WOLFSSL_CBIO_ERR_GENERAL;
        }
    }
    else if (sent == 0) {
        printf("Connection closed\n");
        return 0;
    }

    /* successful send */
    printf("my_IOSend: sent %d bytes to %d\n", sz, sockfd);
    return sent;
}
```

<<<<<<< HEAD
The most complex part of these callbacks are probably their error reporting.
wolfSSL expects more error information that we've been doing in these examples
until now. As such, to be responsible we have to translate the `errno` value
into terms that wolfSSL expects form its I/O functions.

But that's the hard part. Now all that's left is to register these functions
with wolfSSL. We can do this at any time between calling `wolfSSL_CTX_new()`
to get `ctx` and `wolfSSL_new()` to get `ssl`. After we've handled loading
certificate or key files but before the "Initialize the server address struct
with zeros" block is a good place. After this, add these lines:

```c
    /* Register callbacks */
    wolfSSL_SetIORecv(ctx, my_IORecv);
    wolfSSL_SetIOSend(ctx, my_IOSend);
```

And just like that wolfSSL will use our functions to send and receive data. Now
when this program is run we should see a number of "my\_OISend: sent" and
"my\_IORecv: recieved" lines in our output.
=======
We will also need to make some chances to `ClientGreet()` which should now look
like this:

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
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

#### <a name="run-callback">Running</a>

`server-tls-callback` can be connected to by the following:

* `client-tls`
* `client-tls-callback`
* `client-tls-nonblocking`
* `client-tls-resume`
* `client-tls-writedup`

`client-tls-callback` can connect to the following:

* `server-tls`
* `server-tls-callback`
* `server-tls-nonblocking`
* `server-tls-threaded`



## <a name="ecc">Using ECC</a>

We can also have wolfSSL use ECC keys.

#### <a name="server-ecc">Server</a>

We'll modify the server first. Copy `server-tls.c` to a new file,
`server-tls-ecdhe.c`, that we will modify. The finished version can be found
[here][s-tls-e].

The first change will be to our file locations. Change the defines for
`CERT_FILE` and `KEY_FILE` to the following:

<<<<<<< HEAD
=======
Now we need to check for non-blocking in our `Security()` function. To do this,
we change:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a
```c
#define CERT_FILE "../certs/server-ecc.pem"
#define KEY_FILE  "../certs/ecc-key.pem"
```

Furthermore, we'll want to define `CIPHER_LIST`. This will be a list of all
ciphers we support on this server. The define will look like this:

```c
#define CIPHER_LIST "ECDHE-ECDSA-CHACHA20-POLY1305"
```

Finally, all we need to do is set this cipher list. Just after the "Load server
key into `WOLFSSL_CTX`" block, add these lines:

```c
    /* Set cipher list */
    if (wolfSSL_CTX_set_cipher_list(ctx, CIPHER_LIST) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set cipher list\n");
        return -1;
    }
```

<<<<<<< HEAD
And just like that, we're done!

#### <a name="client-ecc">Client</a>

This is one of those cases where we have more to change about the client than
the server. Copy `client-tls.c` to a new file, `client-tls-ecdhe.c`, that we
will modify. The finished version can be found [here][c-tls-e].

We start by changing the definition of `CERT_FILE`. It should now look like
this:

```c
#define CERT_FILE "../certs/server-ecc.pem"
```

After this, we need two more files and a cipher list. In total, these new
defines will look like this:
=======
Next, we need to change the socket to Non-blocking. This is done by adding the
following command after the creation of the socket:

```c
/* sets socket to non-blocking */
fcntl(sockfd, F_SETFL, O_NONBLOCK);
```

Then we will need to delay connection to the port until it finishes rewriting
itself to a non-blocking state. So we have to change how we connect.  Instead
of this:
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

```c
#define ECC_FILE    "../certs/client-ecc-cert.pem"
#define KEY_FILE    "../certs/ecc-client-key.pem"
#define CIPHER_LIST "ECDHE-ECDSA-CHACHA20-POLY1305"
```

With these defined, we just need to load them into `ctx`. We're already doing
this for `CERT_FILE` in the "Load client certificates into `WOLFSSL_CTX`"
block, so add these blocks just after that:

```c
<<<<<<< HEAD
    /* Load client ecc certificates into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_certificate_chain_file(ctx, ECC_FILE) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                ECC_FILE);
        return -1;
    }

    /* Load client ecc key into WOLFSSL_CTX */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)
        != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                KEY_FILE);
        return -1;
    }

    /* Set cipher list */
    if (wolfSSL_CTX_set_cipher_list(ctx, CIPHER_LIST) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to set cipher list\n");
        return -1;
    }
```

And now the client is set up.

#### <a name="run-callback">Running</a>

`server-tls-ecdhe` can be connected to by the following:

* `client-tls-ecdhe`

`client-tls-ecdhe` can connect to the following:

* `server-tls-ecdhe`
=======
/* keeps trying to connect to the socket until it is able to do so */
while (ret != 0)
    ret = connect(sockfd, (struct sockaddr *) &servAddr,
        sizeof(servAddr));
```

This keeps trying to connect to the socket until it stops being busy and allows
the connection.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a



<<<<<<< HEAD
<!-- References -->
[make]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/Makefile

[s-tcp]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tcp.c
[c-tcp]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tcp.c

[s-tls]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tls.c
[c-tls]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tls.c

[s-tls-c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tls-callback.c
[c-tls-c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tls-callback.c
=======
Before running `make` be sure that `SERV_PORT` is the same on both source .c
files (11111 by default).  You must also have the IP address of the machine
where the server is going to be on.

To do this type `ifconfig` in your terminal and look for `inet addr:` under the
`wlan` section.  If you are going to be running the client and server off of
the same machine look under the `lo` section for the local IP address.

Run `make` in both the server and client folders, this will compile the source
files into an executable file. Anytime you make changes to these source files
you will need to re-run `make`. Once this is done you can now start both the
client and server. To start the server navigate to the server folder and type
`./server-tls` and press enter.  To activate the client navigate to the client
folder and type `./client-tls <IP Address>` into your terminal.  This should
work on both the secured and unsecured programs.

Congratulations, you now have a basic client and server that can communicate
with each other in an unsecured or secured manner.
>>>>>>> c15ac5e8a37f1eec257b547000f8b970846bff9a

[s-tls-e]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tls-ecdhe.c
[c-tls-e]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tls-ecdhe.c
