#!/bin/bash

# Copyright (C) 2006-2020 wolfSSL Inc.
#
# This script verifies the DER-encoded PKCS7/CMS bundles created by the
# example applications using the openssl command line tool.
#
# Tested on Ubuntu 17.10 64-bit, using bash shell
#
# Run from the "wolfssl-examples/pkcs7" directory:
#
# $ cd wolfssl-examples/pkcs7
# $ make
# $ ./scripts/openssl-verify.sh
#
# This script relies on the scripts/runall.sh script, and calls it
# automatically to run the PKCS7/CMS example applications which will
# generate the necessary DER-encoded bundles.

EXPECTED_INNER_CONTENT="Hello World"
INNER_CONTENT_FILE="content.txt"
AES256_KEY="0102030405060708010203040506070801020304050607080102030405060708"
AES256_KEYID="02020304"

RSA_RECIP_CERT="../certs/client-cert.pem"
RSA_RECIP_KEY="../certs/client-key.pem"

ECC_RECIP_CERT="../certs/client-ecc-cert.pem"
ECC_RECIP_KEY="../certs/ecc-client-key.pem"


# Check that openssl command exists
if ! [ -x "$(command -v openssl)" ]; then
    echo 'Error: openssl is not installed.' >&2
    exit 1
fi


# Run all example applications that exist
eval "./scripts/runall.sh"

echo ''
echo '---------------------------------------------'
echo 'Testing against installed version of openssl:'
eval 'openssl version'


# Try to verify generated EncryptedData bundle(s)
echo -e "\nVerifying EncryptedData Bundles:"
if [ -f 'encryptedData.der' ]; then
    OUTPUT=$(openssl cms -EncryptedData_decrypt -in encryptedData.der -inform der -secretkey $AES256_KEY)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tencryptedData.der:\t\t\t\tPASSED!'
    else
        echo -e '\tencryptedData.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi


# Trying to verify CompressedData bundle(s)
echo -e "\nVerifying CompressedData Bundles:"
if [ -f 'compressedData.der' ]; then
    OUTPUT=$(openssl cms -uncompress -in compressedData.der -inform der)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tcompressedData.der:\t\t\t\tPASSED!'
    else
        echo -e '\tcompressedData.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi


# Trying to verify EnvelopedData bundle(s), except ORI recipient type
echo -e "\nVerifying EnvelopedData Bundles:"
if [ -f 'envelopedDataKTRI.der' ]; then
    OUTPUT=$(openssl cms -decrypt -in envelopedDataKTRI.der -inform der -recip $RSA_RECIP_CERT -inkey $RSA_RECIP_KEY)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tenvelopedDataKTRI.der:\t\t\t\tPASSED!'
    else
        echo -e '\tenvelopedDataKTRI.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'envelopedDataKARI.der' ]; then
    OUTPUT=$(openssl cms -decrypt -in envelopedDataKARI.der -inform der -recip $ECC_RECIP_CERT -inkey $ECC_RECIP_KEY)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tenvelopedDataKARI.der:\t\t\t\tPASSED!'
    else
        echo -e '\tenvelopedDataKARI.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'envelopedDataKEKRI.der' ]; then
    OUTPUT=$(openssl cms -decrypt -in envelopedDataKEKRI.der -inform der -secretkey $AES256_KEY -secretkeyid $AES256_KEYID)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tenvelopedDataKEKRI.der:\t\t\t\tPASSED!'
    else
        echo -e '\tenvelopedDataKEKRI.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'envelopedDataPWRI.der' ]; then
    OUTPUT=$(openssl cms -decrypt -in envelopedDataPWRI.der -inform der -pwri_password "wolfsslPassword")

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tenvelopedDataPWRI.der:\t\t\t\tPASSED!'
    else
        echo -e '\tenvelopedDataPWRI.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

# Trying to verify AuthEnvelopedData bundle(s) - TODO
echo -e "\nVerifying AuthEnvelopedData Bundles:"
echo -e "\t[SKIPPING - openssl app doesn't support yet]"

# Trying to verify SignedData bundle(s)
echo -e "\nVerifying SignedData Bundles:"
if [ -f 'signedData_attrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_attrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_attrs.der:\t\t\t\tPASSED!'
    else
        echo -e '\tsignedData_attrs.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedData_noattrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_noattrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_noattrs.der:\t\t\t\tPASSED!'
    else
        echo -e '\tsignedData_noattrs.der:\t\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedFirmwarePkgData_attrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedFirmwarePkgData_attrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedFirmwarePkgData_attrs.der:\t\tPASSED!'
    else
        echo -e '\tsignedFirmwarePkgData_attrs.der:\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedFirmwarePkgData_noattrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedFirmwarePkgData_noattrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedFirmwarePkgData_noattrs.der:\t\tPASSED!'
    else
        echo -e '\tsignedFirmwarePkgData_noattrs.der:\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedData_detached_attrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_detached_attrs.der -inform der -CAfile $RSA_RECIP_CERT -content $INNER_CONTENT_FILE 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_detached_attrs.der:\t\t\tPASSED!'
    else
        echo -e '\tsignedData_detached_attrs.der:\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedData_detached_noattrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_detached_noattrs.der -inform der -CAfile $RSA_RECIP_CERT -content $INNER_CONTENT_FILE 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_detached_noattrs.der:\t\tPASSED!'
    else
        echo -e '\tsignedData_detached_noattrs.der:\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedData_cryptodev_attrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_cryptodev_attrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_cryptodev_attrs.der:\t\t\tPASSED!'
    else
        echo -e '\tsignedData_cryptodev_attrs.der:\t\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

if [ -f 'signedData_cryptodev_noattrs.der' ]; then
    OUTPUT=$(openssl cms -verify -in signedData_cryptodev_noattrs.der -inform der -CAfile $RSA_RECIP_CERT 2>/dev/null)

    if [ "$OUTPUT" == "$EXPECTED_INNER_CONTENT" ]; then
        echo -e '\tsignedData_cryptodev_noattrs.der:\t\tPASSED!'
    else
        echo -e '\tsignedData_cryptodev_noattrs.der:\t\tFAILED!'
        echo -e "\t... output = $OUTPUT, expected '$EXPECTED_INNER_CONTENT'"
    fi
fi

