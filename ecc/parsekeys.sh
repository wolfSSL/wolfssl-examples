#!/bin/sh

# SECP256R1
if [ -f ECC_SECP256R1.der ]; then
	openssl ec -inform der -in ECC_SECP256R1.der -text
fi
if [ -f ECC_SECP256R1_pub.der ]; then
	openssl ec -inform der -in ECC_SECP256R1_pub.der -text -pubin
fi

if [ -f ECC_SECP256R1.pem ]; then
	openssl ec -inform pem -in ECC_SECP256R1.pem -text
fi
if [ -f ECC_SECP256R1_pub.pem ]; then
	openssl ec -inform pem -in ECC_SECP256K1_pub.pem -text -pubin
fi


# SECP256K1
if [ -f ECC_SECP256K1.der ]; then
	openssl ec -inform der -in ECC_SECP256K1.der -text
fi
if [ -f ECC_SECP256K1_pub.der ]; then
	openssl ec -inform der -in ECC_SECP256K1_pub.der -text -pubin
fi

if [ -f ECC_SECP256K1.pem ]; then
	openssl ec -inform pem -in ECC_SECP256K1.pem -text
fi
if [ -f ECC_SECP256K1_pub.pem ]; then
	openssl ec -inform pem -in ECC_SECP256K1_pub.pem -text -pubin
fi
