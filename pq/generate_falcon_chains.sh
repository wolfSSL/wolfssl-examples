#!/bin/bash

# Script to generate Falcon NIST Level 1 and 5 certificate chains.
#
# Copyright 2021 wolfSSL Inc. All rights reserved.
# Original Author: Anthony Hu.
#
# Execute this script in the openssl directory after building OQS's fork of
# OpenSSL. Please see the README.md file for more details.

if [ "$OPENSSL" = "" ]; then
   OPENSSL=./apps/openssl
fi

# Generate conf files.
printf "\
[ req ]\n\
prompt                 = no\n\
distinguished_name     = req_distinguished_name\n\
\n\
[ req_distinguished_name ]\n\
C                      = CA\n\
ST                     = ON\n\
L                      = Waterloo\n\
O                      = wolfSSL Inc.\n\
OU                     = Engineering\n\
CN                     = Root Certificate\n\
emailAddress           = root@wolfssl.com\n\
\n\
[ ca_extensions ]\n\
subjectKeyIdentifier   = hash\n\
authorityKeyIdentifier = keyid:always,issuer:always\n\
keyUsage               = critical, keyCertSign\n\
basicConstraints       = critical, CA:true\n" > root.conf

printf "\
[ req ]\n\
prompt                 = no\n\
distinguished_name     = req_distinguished_name\n\
\n\
[ req_distinguished_name ]\n\
C                      = CA\n\
ST                     = ON\n\
L                      = Waterloo\n\
O                      = wolfSSL Inc.\n\
OU                     = Engineering\n\
CN                     = Entity Certificate\n\
emailAddress           = entity@wolfssl.com\n\
\n\
[ x509v3_extensions ]\n\
subjectAltName = IP:127.0.0.1\n\
subjectKeyIdentifier   = hash\n\
authorityKeyIdentifier = keyid:always,issuer:always\n\
keyUsage               = critical, digitalSignature\n\
extendedKeyUsage       = critical, serverAuth,clientAuth\n\
basicConstraints       = critical, CA:false\n" > entity.conf

###############################################################################
# Falcon NIST Level 1
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon_level1_root_key.pem
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon_level1_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 512 -key falcon_level1_root_key.pem -out falcon_level1_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key falcon_level1_entity_key.pem -out falcon_level1_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in falcon_level1_entity_req.pem -CA falcon_level1_root_cert.pem -CAkey falcon_level1_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 513 -out falcon_level1_entity_cert.pem

###############################################################################
# Falcon NIST Level 5
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon_level5_root_key.pem
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon_level5_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 1024 -key falcon_level5_root_key.pem -out falcon_level5_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key falcon_level5_entity_key.pem -out falcon_level5_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in falcon_level5_entity_req.pem -CA falcon_level5_root_cert.pem -CAkey falcon_level5_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 1025 -out falcon_level5_entity_cert.pem

###############################################################################
# Verify all generated certificates.
###############################################################################
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile falcon_level1_root_cert.pem falcon_level1_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile falcon_level5_root_cert.pem falcon_level5_entity_cert.pem

