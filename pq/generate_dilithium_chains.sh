#!/bin/bash

# Script to generate Dilithium NIST Level 2,3 and 5 certificate chains; both
# SHAKE and AES variants.
#
# Copyright 2022 wolfSSL Inc. All rights reserved.
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
# Dilithium NIST Level 2; SHAKE Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium2 -outform pem -out dilithium_level2_root_key.pem
${OPENSSL} genpkey -algorithm dilithium2 -outform pem -out dilithium_level2_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 20 -key dilithium_level2_root_key.pem -out dilithium_level2_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_level2_entity_key.pem -out dilithium_level2_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_level2_entity_req.pem -CA dilithium_level2_root_cert.pem -CAkey dilithium_level2_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 21 -out dilithium_level2_entity_cert.pem

###############################################################################
# Dilithium NIST Level 3; SHAKE Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium3 -outform pem -out dilithium_level3_root_key.pem
${OPENSSL} genpkey -algorithm dilithium3 -outform pem -out dilithium_level3_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 30 -key dilithium_level3_root_key.pem -out dilithium_level3_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_level3_entity_key.pem -out dilithium_level3_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_level3_entity_req.pem -CA dilithium_level3_root_cert.pem -CAkey dilithium_level3_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 31 -out dilithium_level3_entity_cert.pem

###############################################################################
# Dilithium NIST Level 5; SHAKE Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium5 -outform pem -out dilithium_level5_root_key.pem
${OPENSSL} genpkey -algorithm dilithium5 -outform pem -out dilithium_level5_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 50 -key dilithium_level5_root_key.pem -out dilithium_level5_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_level5_entity_key.pem -out dilithium_level5_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_level5_entity_req.pem -CA dilithium_level5_root_cert.pem -CAkey dilithium_level5_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 51 -out dilithium_level5_entity_cert.pem

###############################################################################
# Dilithium NIST Level 2; AES Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium2_aes -outform pem -out dilithium_aes_level2_root_key.pem
${OPENSSL} genpkey -algorithm dilithium2_aes -outform pem -out dilithium_aes_level2_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 20 -key dilithium_aes_level2_root_key.pem -out dilithium_aes_level2_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_aes_level2_entity_key.pem -out dilithium_aes_level2_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_aes_level2_entity_req.pem -CA dilithium_aes_level2_root_cert.pem -CAkey dilithium_aes_level2_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 21 -out dilithium_aes_level2_entity_cert.pem

###############################################################################
# Dilithium NIST Level 3; AES Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium3_aes -outform pem -out dilithium_aes_level3_root_key.pem
${OPENSSL} genpkey -algorithm dilithium3_aes -outform pem -out dilithium_aes_level3_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 30 -key dilithium_aes_level3_root_key.pem -out dilithium_aes_level3_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_aes_level3_entity_key.pem -out dilithium_aes_level3_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_aes_level3_entity_req.pem -CA dilithium_aes_level3_root_cert.pem -CAkey dilithium_aes_level3_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 31 -out dilithium_aes_level3_entity_cert.pem

###############################################################################
# Dilithium NIST Level 5; AES Variant
###############################################################################

# Generate root key and entity private keys.
${OPENSSL} genpkey -algorithm dilithium5_aes -outform pem -out dilithium_aes_level5_root_key.pem
${OPENSSL} genpkey -algorithm dilithium5_aes -outform pem -out dilithium_aes_level5_entity_key.pem

# Generate the root certificate
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 365 -set_serial 50 -key dilithium_aes_level5_root_key.pem -out dilithium_aes_level5_root_cert.pem

# Generate the entity CSR.
${OPENSSL} req -new -config entity.conf -key dilithium_aes_level5_entity_key.pem -out dilithium_aes_level5_entity_req.pem

# Generate the entity X.509 certificate.
${OPENSSL} x509 -req -in dilithium_aes_level5_entity_req.pem -CA dilithium_aes_level5_root_cert.pem -CAkey dilithium_aes_level5_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 365 -set_serial 51 -out dilithium_aes_level5_entity_cert.pem

###############################################################################
# Verify all generated certificates.
###############################################################################
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_level2_root_cert.pem dilithium_level2_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_level3_root_cert.pem dilithium_level3_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_level5_root_cert.pem dilithium_level5_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_aes_level2_root_cert.pem dilithium_aes_level2_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_aes_level3_root_cert.pem dilithium_aes_level3_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile dilithium_aes_level5_root_cert.pem dilithium_aes_level5_entity_cert.pem

