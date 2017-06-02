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

0. [Running these examples](#run)
0. [Compiling these examples](#compile)
0. [A simple TCP client/server pair](#tcp)
    0. [Server](#server-tcp)
    0. [Client](#client-tcp)
    0. [Running](#run-tcp)
0. [Converting to use wolfSSL](#tls)
    0. [Server](#server-tls)
    0. [Client](#client-tls)
    0. [Running](#run-tls)

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

```c
    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */
```

That initialize-with-zeros step is not strictly necessary, but it's usually a
good idea, and it doesn't complicate the example too much.

Hopefully those comments illuminate what everything means.

With the address all filled out, the final stage of setting up the server is to
bind our socket to a port and start listening for connections. In total, this
should look like this:

```c
    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        return -1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        return -1;
    }
```

`bind()` makes it such that our server on `sockfd` is now visible at the
location described by `servAddr`, and `listen()` causes us to start listening
to `sockfd` for incoming client connections.

And now the setup is complete. Now we just need to deal with I/O.

We're going to keep accepting clients until one of them tells us "shutdown". To
start, let's write up a quick skeleton for this part of the code:

```c
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept clients here */

        printf("Client connected successfully\n");



        /* Do comunication here */



        /* Cleanup after this connection */
    }

    printf("Shutdown complete\n");
```

We'll deal with accepting clients first. Replace the "Accept clients here"
comment with these lines:

```c
        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            fprintf(stderr, "ERROR: failed to accept the connection\n\n");
            return -1;
        }
```

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

This zeros out the buffer, then gets a message from the client and prints it
to `stdout` so we can see it. Recall that we want to shutdown when the client
tells us "shutdown". To accomplish this, add these lines after we print the
message:

```c
        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }
```

Note that it doesn't end execution or break out of the loop right away. We
still want to respond to the client with our own message.

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

And we're done.

We've set up a server on a TCP socket and dealt with a quick back-and-forth
with a client.

Now we just need a client.

#### <a name="client-tcp">Client</a>

Now we'll write the client. We'll be rewriting `client-tcp.c` from scratch, so
there's no reason to copy anything. The finished version can be found
[here][c-tcp].

Once more, we'll start with a skeleton:

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
    }
```

We expect exactly two arguments: our name and an IPv4 address. We won't be
confirming that the second argument is a well formed IPv4 address, though. If
it's not, we'll error out midway through.

Now that we know the program has been called more-or-less correctly, we'll open
a socket to represent our client:

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

Now we can fill out the address of the server we want to connect to. After the
"Create a socket [...]" block, add these lines:

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

And that's everything! Our client will just be a quick one-and-done thing.

#### <a name="run-tcp">Running</a>

`server-tcp` can be connected to by the following:

* `client-tcp.c`

`client-tcp` can connect to the following:

* `server-tcp.c`

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

```c
    /* Initialize wolfSSL */
    wolfSSL_Init();
```

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

Recall that `CERT_FILE` is one of our own defines.

What the above does is load the certificate file `CERT_FILE` so that the server
can verify itself to clients. This certificate is known to be in the PEM format
(indicated by `SSL_FILETYPE_PEM`), and this information is then kept in `ctx`.

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

And that's everything for the setup phase. Now we just need add a few things to
how we handle clients.

After we accept a new client, we need to make a wolfSSL object. Just after the
"Accept client connections" block, add these lines:

```c
        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
            return -1;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);
```

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

Second, we need to do something similar to our calls to `write()`. The "Reply
back to the client" block now should look like this:

```c
        /* Reply back to the client */
        if (wolfSSL_write(ssl, buff, len) != len) {
            fprintf(stderr, "ERROR: failed to write\n");
            return -1;
        }
```

And after this, we've successfully modified our TCP server to use wolfSSL for
TLS 1.2 connections. Now we just need a client to connect with.

#### <a name="client-tls">Client</a>

Now we'll write the client. Copy `client-tcp.c` to a new file, `client-tls.c`,
that we will modify. The finished version can be found [here][c-tls].

Like the server, the first thing to do is include the wolfSSL header. Just
below the "socket includes" block, add these lines:

```c
/* wolfSSL */
#include <wolfssl/ssl.h>
```

We're going to need a different definition for `CERT_FILE`, however. Just below
the definition of `DEFAULT_PORT` add this line:

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

And now we can start hooking up wolfSSL. The first thing to do is initialize
the wolfSSL library. This time we'll do it after we check our calling
convention. Beneath the "Check for proper calling convention" block, add the
lines:

```c
    /* Initialize wolfSSL */
    wolfSSL_Init();
```

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
`CERT_FILE` into `ctx` so that we can verify the identity of the servers we
connect to. It should be noted that that third argument, here `NULL`, is a path
to search for certificate files. It should also be noted that all files are
assumed to be in the PEM format.

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

And this concludes the setup. Now we just need to deal with the connection to
the server. After the "Connect to the server" block, add these lines:

```c
    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        return -1;
    }

    /* Attach wolfSSL to the socket */
    wolfSSL_set_fd(ssl, sockfd);

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

* `client-tls.c`
* `client-tls-callback.c`
* `client-tls-ecdhe.c`
* `client-tls-nonblocking.c`
* `client-tls-resume.c`
* `client-tls-writedup.c`

`client-tls` can connect to the following:

* `server-tls.c`
* `server-tls-callback.c`
* `server-tls-ecdhe.c`
* `server-tls-nonblocking.c`
* `server-tls-threaded.c`



<!-- References -->
[make]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/Makefile

[s-tcp]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tcp.c
[c-tcp]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tcp.c

[s-tls]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tls.c
[c-tls]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tls.c
