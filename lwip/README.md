# LWIP Native Example

Assumes there is an existing STM project with native LwIP support. wolfSSL needs to be compiled with the macro `WOLFSSL_LWIP_NATIVE`

The main function to call in the example code is `tls_echoclient_connect();`

Macros in `lwipopts.h` were also adjusted to better handle TLS packets, which can be as large as 16k. The macros adjusted were:
- MEM_SIZE(16*1024)
- MEMP_NUM_PBUF (10 -> 20)
- PBUF_POOL_SIZE (8 -> 9)
- PBUF_POOL_BUFSIZE (512 -> 2048)
- TCP_WND (2*TCP_MSS -> 11*TCP_MSS)


To start up a server for the example client to connect to use the bundled wolfSSL example
server on a desktop:

```
cd wolfssl
./configure && make
./examples/server/server -b -d -p 11111 -c ./certs/server-ecc.pem -k ./certs/ecc-key.pem -i
```
