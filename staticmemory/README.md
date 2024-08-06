Examples of using wolfSSL's simple memory allocator. Enabled with --enable-staticmemory
or WOLFSSL_STATIC_MEMORY when building wolfSSL. This is not a feature to reduce
memory size!! It in fact uses a little more memory overhead. The staticmemory
feature is used to have a simple fixed pool of memory available instead of using
a systems malloc/free calls. Helpful in cases where dynamic memory is not
allowed.

wolfSSL has support for XMALLOC_USER which could be used to instead map XMALLOC
and XFREE to any desired implementation of malloc/free.
