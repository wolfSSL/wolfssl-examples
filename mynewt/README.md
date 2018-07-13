Apache Mynewt examples
=============

## Overview

It provides follows mynewt packages.

- apps/wolfsslclienttlsmn
    - client-tls simple example ssl/tls client application for mn_socket.

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
