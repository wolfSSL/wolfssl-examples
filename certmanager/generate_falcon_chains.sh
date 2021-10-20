#!/bin/bash 

# Script to generate Falcon 512 and Falcon1024 certificate chains.
#
# Copyright 2021 wolfSSL Inc. All rights reserved.
# Original Author: Anthony Hu.
#
# Execute this script in the openssl directory after building OQS's fork of
# OpenSSL. Please see the README.md file for more details.

if [[ ! -v OPENSSL ]]; then
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
subjectKeyIdentifier   = hash\n\
authorityKeyIdentifier = keyid:always,issuer:always\n\
keyUsage               = critical, digitalSignature\n\
extendedKeyUsage       = critical, serverAuth,clientAuth\n" > entity.conf

###############################################################################
# Falcon 512
###############################################################################

# Generate root key and entity private keys. 
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon512_root_key.pem
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon512_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 512 -key falcon512_root_key.pem -out falcon512_root_cert.pem 

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key falcon512_entity_key.pem -out falcon512_entity_req.pem 

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in falcon512_entity_req.pem -CA falcon512_root_cert.pem -CAkey falcon512_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 513 -out falcon512_entity_cert.pem

###############################################################################
# Falcon 1024
###############################################################################

# Generate root key and entity private keys. 
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon1024_root_key.pem
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon1024_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 1024 -key falcon1024_root_key.pem -out falcon1024_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key falcon1024_entity_key.pem -out falcon1024_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in falcon1024_entity_req.pem -CA falcon1024_root_cert.pem -CAkey falcon1024_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 1025 -out falcon1024_entity_cert.pem

###############################################################################
# Verify all generated certificates.
###############################################################################
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile falcon512_root_cert.pem falcon512_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile falcon1024_root_cert.pem falcon1024_entity_cert.pem

