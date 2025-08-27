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
echo "# Generate ECC example"
./pkcs11_genecc /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# AES-GCM example"
./pkcs11_aesgcm /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# AES-CBC example"
./pkcs11_aescbc /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# HMAC example"
./pkcs11_hmac /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# Random Number Generation example"
./pkcs11_rand /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# PKCS #11 test"
./pkcs11_test /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
echo
echo "# PKCS11 CSR example"
./pkcs11_csr /usr/local/lib/opencryptoki/libopencryptoki.so 3 SoftToken cryptoki
