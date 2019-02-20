#!/bin/sh

openssl ec -inform der -in ECC_SECP256K1.der -text
openssl ec -inform pem -in ECC_SECP256K1.pem -text

openssl ec -inform der -in ECC_SECP256K1_pub.der -text -pubin
openssl ec -inform pem -in ECC_SECP256K1_pub.pem -text -pubin
