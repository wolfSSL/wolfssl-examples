# Apache Mynewt examples

## Overview

[Mynewt](https://mynewt.apache.org) is a security-oriented OS for IoT.

WolfSSL integration with the Mynewt OS may be found in the `/IDE/mynewt`
directory in the `wolfssl` source-tree.  Support includes the wolfCrypt test
application which demonstrates the cryptography provided by the wolfCrypt
component of wolfSSL.

`wolfssl` provides the following mynewt packages:

- _crypto/wolfsslwolfssl_ - wolfSSL SSL/TLS and wolfCrypt cryptography library
- _apps/wolfcrypttest_ - wolfCrypt unit test application

The examples for wolfSSL and Mynewt found in the `mynewt` directory of
`wolfssl-examples` (this directory) includes demonstration of additional
wolfSSL capabilities such as TLS.

`wolfssl-examples` provides the follows mynewt packages:

- _apps/wolfsslclienttlsmn_ - simple client TLS application for `mn_socket`

The Mynewt OS has a primary command-line tool called `newt` which is used to
configure and build Mynewt for targets.  Targets include a simulated environment
for Linux hosts, which will be used by the wolfSSL applications.  Mynewt installs
to a project directory from which the `newt` command is run.

## Setup

1. Install Mynewt `newt` command using instructions found at:

http://mynewt.apache.org/latest/get_started/native_install/index.html

2. Create Mynewt project directory (e.g. `mynewt`).  

In parent folder of new Mynewt project directory:

```
$ newt new mynewt
```

NOTE: If problems are encountered creating a new Mynewt project an alternative:

```
$ git clone git@github.com:apache/mynewt-blinky.git mynewt
$ cd mynewt
$ newt upgrade
```

3. Install wolfSSL support into Mynewt project.

In the `wolfssl/IDE/mynewt` directory:

```
$ ./setup path-to-mynewt
```

4. Install wolfSSL examples support into Mynewt project.  

In the `wolfssl-examples/mynewt` directory:

```
$ ./setup path-to-mynewt
```

## Configuration

1. Create client TLS application for simulator target.

In Mynewt project directory:

```
$ newt target create wolfsslclienttlsmn_sim
$ newt target set wolfsslclienttlsmn_sim app=apps/wolfsslclienttlsmn
$ newt target set wolfsslclienttlsmn_sim bsp=@apache-mynewt-core/hw/bsp/native
$ newt target set wolfsslclienttlsmn_sim build_profile=debug
```

## Build and Run

1. Build client TLS application.

In Mynewt project directory:

```
$ newt clean wolfsslclienttlsmn_sim
$ newt build wolfsslclienttlsmn_sim
```

2. Run client TLS application

In Mynewt project directory:

```
$ ./bin/targets/wolfsslclienttlsmn_sim/app/apps/wolfsslclienttlsmn/wolfsslclienttlsmn.elf
```

The Mynewt simulator will display the linux host device where the application's
uart is accessible via a terminal emulator program such as `picocom`, `screen` or
`kermit`.  For example: `uart0 at /dev/pts/3`

3. Connect terminal emulator (e.g. picocom) to simulated application UART.

```
$ sudo picocom -b 115200 /dev/pts/3
```

The application will display the Mynewt shell prompt `compat>` (press ENTER if
shell prompt is not visiable).  Entering `help` will display commands available,
including the `wolfssl` command with represents the client TLS application.

## Usage

### Command list

The client TLS application `wolfssl` has the following commands:


| command | argument                         | describe                   | example                               |
|---------|----------------------------------|----------------------------|---------------------------------------|
| time    | "unix timestamp"                 | To set the time            | "time 1532616682"                     |
| net     | udp                              | create udp socket          | "net udp"                             |
| net     | tcp                              | create tcp socket          | "net tcp"                             |
| net     | connect "ipaddress" port         | connect "ipaddress"        | "net connect 93.184.216.34 443"       |
| net     | close                            | close socket               | "net close"                           |
| net     | send "string" "ipaddress" "port" | send string                | "net send "GET \r\n" 93.184.216.34 80 |
| net     | recv "ipaddress"                 | recv from ipaddress        | "net recv 93.184.216.34 80            |
| wolfssl | init                             | initialize wolfssl library | "wolfssl init"                        |
| wolfssl | connect                          | connect via ssl            | "wolfssl connect"                     |
| wolfssl | write "string"                   | send string via ssl        | "wolfssl write "GET /""               |
| wolfssl | read                             | recv via ssl               | "wolfssl recv"                        |
| wolfssl | clear                            | finish wolfssl library     | "wolfssl clear"                       |

### Command examples
Get `index.html` from `www.example.com:443` (i.e. `93.184.216.34:443`) using
Mynewt TCP networking and the wolfSSL TLS and crypto.

At the Mynewt `compat>` shell prompt:

```
net tcp
net connect 93.184.216.34 443
wolfssl init
wolfssl connect
wolfssl write "GET /"
wolfssl read
wolfssl clear
net close
```

The resulting application output should be similar to the following:

```
compat> net tcp
001143 mn_socket(TCP) = 0 566b7800
compat> net connect 93.184.216.34 443
005078 93.184.216.34/443
005078 mn_connect() = 0
compat> net_test_writable 0 - 0
wolfssl init
005853 wolfssl contexts are initialized
005854 wolfSSL ctx initialize
compat> wolfssl connect
006517 wolfSSL_connect() = 1
compat> wolfssl write "GET /"
009182 wolfSSL_write() = 4L
compat> wolfssl read
010564 HTTP/1.0 501 Not Implemented
Content-Type: text/html
Content-Length: 357
Connection: close
Date: Wed, 12 Apr 2023 14:49:27 <?xml version="1.0" encoding="iso-8859-1"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
         "http://wwwitle>501 - Not Implemented</title>
	</head>
	<body>
		<h1>501 - Not Implemented</h1>
	</body>
</html>

010578 
010578 ERROR: wolfSSL_read rc:-1 err:6
compat> wolfssl clear
012551 clear wolfssl contexts
012553 wolfSSL ctx clear
compat> net close
012893 mn_close() = 0
compat> 
```

NOTE: The server-side connection close after reception of data results in the 
      read error.

## Notes

- Client TLS example run on _Ubuntu 22.04LTS_ host with _wolfSSL v5.6.0_.

- See `wolfssl/IDE/mynewt/README.md` for details on wolfSSL integration and use
  of the wolfCrypt test application.


# Jenkins script (test automation)

## Setup for jenkins node

Install:

- git
- expect
- bash
- screen
- [newt](https://mynewt.apache.org/latest/get_started/native_install/index.html)(v1.4.1 over)

## Usage

Execute `jenkins.sh` script on jenkins.

```
./mynewt/jenkins.sh
```
