#!/bin/bash

# Copyright (C) 2006-2020 wolfSSL Inc.
#
# This script runs all PKCS7/CMS example applications that have been
# compiled and are located in the "wolfssl-examples/pkcs7" directory.
#
# Tested on Ubuntu 17.10 64-bit, using bash shell
#
# Run from the "wolfssl-examples/pkcs7" directory:
#
# $ cd wolfssl-examples/pkcs7
# $ make
# $ ./scripts/runall.sh
#
# This script is called by the openssl-verify.sh script, which will then
# verify the DER-encoded PKCS7/CMS bundles generated by the example apps.

fileArray=(
    # CMS AuthEnvelopedData example apps
    "authEnvelopedData-kari"
    "authEnvelopedData-kekri"
    "authEnvelopedData-ktri"
    "authEnvelopedData-ori"
    "authEnvelopedData-pwri"

    # CMS CompressedData example apps
    "compressedData"

    # CMS EncryptedData example apps
    "encryptedData"

    # CMS EnvelopedData example apps
    "envelopedData-kari"
    "envelopedDataDecode"
    "envelopedData-kekri"
    "envelopedData-ktri"
    "envelopedData-ori"
    "envelopedData-pwri"

    # CMS SignedData example apps
    "signedData"
    "signedData-CompressedFirmwarePkgData"
    "signedData-EncryptedCompressedFirmwarePkgData"
    "signedData-EncryptedFirmwarePkgData"
    "signedData-EncryptedFirmwareCB"
    "signedData-FirmwarePkgData"
    "signedData-DetachedSignature"
    "signedData-cryptodev"
    )

echo "Running example applications..."
echo ""
for i in "${fileArray[@]}"
do
    if [ -f $i ]; then
        if [ "$i" == "envelopedDataDecode" ]; then
            eval "./$i ../certs/client-cert.der ../certs/client-key.der envelopedDataKTRI.der"
        else
            eval "./$i"
        fi
        if [ $? -ne 0 ]
        then
            echo "Test FAILED"
            exit
        fi
    fi
done

echo ""
echo "All examples finished successfully"

