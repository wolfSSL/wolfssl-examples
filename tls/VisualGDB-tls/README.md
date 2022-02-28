# wolfSSL TLS Examples in Visual Studio

These examples use the VisualGDB extension for Visual Studio and assume WSL is already installed with toolchain.

Basic steps to create a new wolfSSL example:

Step 1:

![VisualGDB-New-Project-Step1.png](./images/VisualGDB-New-Project-Step1.png)

Step 2:

![VisualGDB-New-Project-Step2.png](./images/VisualGDB-New-Project-Step2.png)

Step 3:

![VisualGDB-New-Project-Step3.png](./images/VisualGDB-New-Project-Step3.png)

Step 4:

![VisualGDB-New-Project-Step4.png](./images/VisualGDB-New-Project-Step4.png)

Step 5:

![VisualGDB-New-Project-Step5.png](./images/VisualGDB-New-Project-Step5.png)

Step 6:

![VisualGDB-New-Project-Step6.png](./images/VisualGDB-New-Project-Step6.png)

Step 7:

![VisualGDB-New-Project-Step7.png](./images/VisualGDB-New-Project-Step7.png)

All of the code refers to source in `..\..\tls`


## Troubleshooting


### Failed to start GDB session

![failed_to_start_gdb_session.png](./images/failed_to_start_gdb_session.png)

Make sure to build the projhect before single-step debugging.


### error adding symbols: DSO missing from command line

If when compiling:

```sh
clang++ -o Debug/memory-tls -Wl,-gc-sections   -Lwolfssl -Wl,--start-group Debug/memory-tls.o -lwolfssl  -Wl,--rpath='$ORIGIN'   -Wl,--end-group
```


```text
/usr/bin/ld: Debug/memory-tls.o: undefined reference to symbol 'pthread_create@@GLIBC_2.2.5'
/usr/bin/ld: /lib/x86_64-linux-gnu/libpthread.so.0: error adding symbols: DSO missing from command line
clang: error: linker command failed with exit code 1 (use -v to see invocation)
```

Be sure to set `-pthread` in Makefile settings - Common Flags:

![common_flags-pthread.png](./images/common_flags-pthread.png)


## Firewall

May need to allow Windows Firewall to listen on GDB ports:

![VisualStudio-firewall.png](./images/VisualStudio-firewall.png)