#!/bin/sh

if [ $# -gt 0 ]
then
  SOFTHSM2_SLOTID=$1
fi

echo "# Using slot ID: $SOFTHSM2_SLOTID"
echo
echo "# RSA example"
./pkcs11_rsa /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# ECC example"
./pkcs11_ecc /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# Generate ECC example"
./pkcs11_genecc /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# AES-GCM example"
./pkcs11_aesgcm /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# AES-CBC example"
./pkcs11_aescbc /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# HMAC example"
./pkcs11_hmac /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# Random Number Generation example"
./pkcs11_rand /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki
echo
echo "# PKCS#11 test"
./pkcs11_test /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki

