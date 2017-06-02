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
procedure. It cannot be guaranteed that these programs will will be free of
memory leaks, especially in error conditions.

Tutorial
========

This portion of the `README` is dedicated to walking you through creating most
of the files in this directory. Before we begin, a note: all references to
files are made relative to this `tls/` directory. Any file reference should
then be modified to point to the correct location. It is also recommended that
you make a new directory to write your files into. If you make a new
subdirectory in the `tls/` directory, simply append an extra `../` to the front
of any file path. Another excellent choice is to make a new directory in the
root directory of this repository, such that there is no need to change the
file path.

## Table of contents

1. [A simple TCP client/server pair](#tcp)
  1. [Server](#server-tcp)
<!-- TODO: table of contents -->

## <a name="tcp">A simple TCP client/server pair</a>

We'll begin by making a simple client/server pair that will communicate in
plaintext over a TCP socket.

### <a name="server-tcp">Server<\a>

We'll write the server first. We'll be rewriting [server-tcp.c]



<!-- References -->
[server-tcp.c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tcp.c
[client-tcp.c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tcp.c

[server-tls.c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/server-tls.c
[client-tls.c]: https://github.com/wolfssl/wolfssl-examples/blob/master/tls/client-tls.c
