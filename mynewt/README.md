Apache Mynewt examples
=============

## Overview

It provides follows mynewt packages.

- apps/wolfsslclienttlsmn
    - client-tls simple example ssl/tls client application for mn_socket.

## Prepare

install `newt` command by referencing below document.

http://mynewt.apache.org/latest/get_started/native_install/index.html

## How to setup

### delopy wolfssl source to mynewt project
Specify the path of the mynewt project and execute  `mynewt/setup.sh`.

```bash
./mynewt/setup.sh　/path/to/myproject_path
```

This script will deploy wolfssl's mynewt package described in the Overview to the mynewt project.

## example setting example applications to targets

This section eplains how to set wolfssl example application to target device.

Please execute follows commands at only once.

create wolfsslclienttlsmn_sim that is simulator device for unit test of client-tls.

```
cd /path/to/myproject_path
newt target delete wolfsslclienttlsmn_sim
newt target create wolfsslclienttlsmn_sim
newt target set wolfsslclienttlsmn_sim app=apps/wolfsslclienttlsmn
newt target set wolfsslclienttlsmn_sim bsp=@apache-mynewt-core/hw/bsp/native
newt target set wolfsslclienttlsmn_sim build_profile=debug
```


## build & test

build and execute wolfsslclienttlsmn_sim

```
newt clean wolfsslclienttlsmn_sim
newt build wolfsslclienttlsmn_sim
./bin/targets/wolfsslclienttlsmn_sim/app/apps/wolfsslclienttlsmn/wolfsslclienttlsmn.elf
```

## Usage
### connect `wolfsslclienttlsmn`

`wolfssl clienttlsmn.elf` displays tty file path.
be able to connect `wolfsslclienttlsmn.elf` by using terminal softwre such as `screen` or `kermit`.

### command list

`wolfsslclienttlsmn` has below commands.


| command | argument                         | describe                   | example                               |
|---------|----------------------------------|----------------------------|---------------------------------------|
| time    | "unix timestamp"                 | To set the time            | "time 1532616682"                     |
| net     | udp                              | create udp socket          | "net udp"                             |
| net     | tcp                              | create tcp socket          | "net tcp"                             |
| net     | connect "ipaddress" port         | connect "ipaddress"        | "net connect 93.184.216.34" 443       |
| net     | close                            | close socket               | "net close"                           |
| net     | send "string" "ipaddress" "port" | send string                | "net send "GET \r\n" 93.184.216.34 80 |
| net     | recv "ipaddress"                 | recv from ipaddress        | "net recv 93.184.216.34 80            |
| wolfssl | init                             | initialize wolfssl library | "wolfssl init"                        |
| wolfssl | connect                          | connect via ssl            | "wolfssl connect"                     |
| wolfssl | write "string"                   | send string via ssl        | "wolfssl write "GET /""               |
| wolfssl | read                             | recv via ssl               | "wolfssl recv"                        |
| wolfssl | clear                            | finish wolfssl library     | "wolfssl clear"                       |

### command examples
get `index.html` from `www.example.com:443`

```
net tcp
net connect
wolfssl init
wolfssl connect
wolfssl write
wolfssl read
wolfssl clear
net close
```
