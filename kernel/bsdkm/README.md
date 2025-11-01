# FreeBSD wolfcrypt kernel module example

## About


Tested on FreeBSD 14.2:
```sh
uname -rsm
FreeBSD 14.2-RELEASE amd64
```

## Build libwolfssl.ko

```sh
cd ~/
git clone https://github.com/philljj/wolfssl.git
cd ~/wolfssl && git co bsdkm
./autogen.sh
./configure --enable-bsdkm --enable-cryptonly --enable-crypttests --enable-all-crypto --disable-asm && make
file bsdkm/libwolfssl.ko
```

Load the kernel module:
```sh
sudo kldload bsdkm/libwolfssl.ko
```

In dmesg output you should see something like:
```sh
dmesg | tail -n10
PKCS7enveloped  test passed!
PKCS7authenveloped  test passed!
mp       test passed!
prime    test passed!
logging  test passed!
mutex    test passed!
crypto callback test passed!
Test complete
wolfCrypt self-test passed.
info: wolfkmod init good
```

and kldstat:
```sh
kldstat
Id Refs Address                Size Name
 1   20 0xffffffff80200000  1f3c6c0 kernel
 2    1 0xffffffff82818000     3220 intpm.ko
 3    1 0xffffffff8281c000     2178 smbus.ko
 4    1 0xffffffff8281f000     430c virtio_console.ko
 5    1 0xffffffff82824000     3360 uhid.ko
 6    1 0xffffffff82828000     3360 wmt.ko
17    1 0xffffffff8282c000   154520 libwolfssl.ko
```

wolfssl will also appear in vmstat entries:
```sh
vmstat -m | grep wolf
         wolfssl    0     0 1275500 16,32,64,128,256,384,512,1024,2048,4096,8192,16384
```

## Build this example

From this example dir:
```sh
make && file bsd_example.ko
```

Load it:
```sh
sudo kldload ./bsd_example.ko
```

dmesg should show:
```sh
dmesg | tail -n5
Test complete
wolfCrypt self-test passed.
info: wolfkmod init good
info: bsdkm_example: running wc_aes_test()
info: bsdkm_example: wc_aes_test good
```

and kldstat:
```sh
kldstat
Id Refs Address                Size Name
 1   22 0xffffffff80200000  1f3c6c0 kernel
 2    1 0xffffffff82818000     3220 intpm.ko
 3    1 0xffffffff8281c000     2178 smbus.ko
 4    1 0xffffffff8281f000     430c virtio_console.ko
 5    1 0xffffffff82824000     3360 uhid.ko
 6    1 0xffffffff82828000     3360 wmt.ko
17    2 0xffffffff8282c000   154520 libwolfssl.ko
18    1 0xffffffff82981000     2188 bsd_example.ko
```

Notice `libwolfssl.ko` reference count has incremented.

Unload in the opposite order as loading:
```sh
sudo kldunload bsd_example.ko
sudo kldunload libwolfssl.ko
kldstat
Id Refs Address                Size Name
 1   18 0xffffffff80200000  1f3c6c0 kernel
 2    1 0xffffffff82818000     3220 intpm.ko
 3    1 0xffffffff8281c000     2178 smbus.ko
 4    1 0xffffffff8281f000     430c virtio_console.ko
 5    1 0xffffffff82824000     3360 uhid.ko
 6    1 0xffffffff82828000     3360 wmt.ko
```
wolfssl should now have disappeared from the vmstat listing:

```sh
# returns nothing
vmstat -m | grep wolf
```
