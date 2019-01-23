#!/bin/sh

echo "# Note opencryptoki does not support ECC not AES-GCM but operations"
echo "# are performed in software"
echo
echo "# RSA example"
./pkcs11_rsa /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# ECC example"
./pkcs11_ecc /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# AES-GCM example"
./pkcs11_aesgcm /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# PKCS #11 test"
./pkcs11_test /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki


