# Description

Simple example of wolfCrypt ML-DSA signing and verifying.

Requires wolfSSL is built with:

```sh
./configure --enable-dilithium
make
make install
```

Build the ML-DSA example with:

```sh
make ml_dsa_test
```

# Usage

```
./ml_dsa_test -?
usage:
  ./ml_dsa_test [-pvw] -c <security category> [-m <message>]

parms:
  -m <message>               the message to sign
  -c <security category>     set the security category {2,3,5}
  -p                         print FIPS 204 parameters and exit
  -v                         verbose
  -w                         write keys, msg, and sig to file
  -?                         show this help
```

Signing and verifying a test message with Category 5, and writing the keys,
message, and signature to `*.key` and `*.bin` files:
```
./ml_dsa_test -c 5 -m "my test message" -w
info: making key
info: using ML-DSA-87, Security Category 5: priv_len 4896, pub_len 2592, sig_len 4627
info: signed message
info: verify message good
info: done
```

The supported ML-DSA parameters:

```
./ml_dsa_test -p
ML-DSA parameters and key/sig sizes*

                Private Key   Public Key   Signature Size   Security Strength
    ML-DSA-44      2560          1312         2420            Category 2
    ML-DSA-65      4032          1952         3309            Category 3
    ML-DSA-87      4896          2592         4627            Category 5


* from Tables 1 & 2 of FIPS 204:
    https://csrc.nist.gov/pubs/fips/204/final
```
