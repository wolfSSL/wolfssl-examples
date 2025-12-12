This is a collection of examples using the wolfSSL Ada wrapper

By default it is expecting the wolfssl directory to be located at wolfssl-examples/Ada/wolfssl
but that can be adjusted by changing the directory path in wolfssl_sha256.gpr
`with "../wolfssl/wrapper/Ada/config/wolfssl_config.gpr";` and the for Source_Dirs paths.

An example of building for use with SHA256 would be:

```
cd wolfssl-examples/Ada
git clone --depth=1 git@github.com:wolfssl/wolfssl
cd sha256
gprbuild sha256.gpr
```

And then running the example with:

```
bash-3.2$ ./obj/sha256_main
DFFD6021BB2BD5B0AF676290809EC3A53191DD81C7F70A4B28688A362182986F
```

Which matches the hash used from shasum:

```
bash-3.2$ printf 'Hello, World!' | shasum -a 256
dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f  -
```
