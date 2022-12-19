# ECH Example that connects to a cloudflare test server which supports ech

This test is successful if the cloudflare http response shows that `sni=encrypted`.

```sh
make
./ech_cloudflare_test 
wolfSSL error: Error loading ca-cert.pem
[jack@archlaptop ech]$ vim cert.pem
[jack@archlaptop ech]$ make
make: 'ech_cloudflare_test' is up to date.
[jack@archlaptop ech]$ ./ech_cloudflare_test 
HTTP/1.1 200 OK
Access-Control-Allow-Origin: *
Cache-Control: no-cache
Cf-Ray: 77c3e3e937c6b08e-ATL
Content-Type: text/plain
Expires: Thu, 01 Jan 1970 00:00:01 GMT
Server: cloudflare
X-Content-Type-Options: nosniff
X-Frame-Options: DENY
Date: Mon, 19 Dec 2022 23:24:11 GMT
Transfer-Encoding: chunked

106
fl=507f46
h=crypto.cloudflare.com
ip=173.93.184.37
ts=1671492251.082
visit_scheme=https
uag=Mozilla/5.0 (X11; Linux x86_64; rv:105.0) Gecko/20100101 Firefox/105.0
colo=ATL
sliver=none
http=http/1.1
loc=US
tls=TLSv1.3
sni=encrypted
warp=off
gateway=off
kex=P-256

0
```
