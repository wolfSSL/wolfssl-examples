#!/bin/sh

# set this to point to the wolfSSL root directory for updating certs
WOLFSSL_ROOT_DIR=~/clean-wolfssl

cp ${WOLFSSL_ROOT_DIR}/certs/client-cert.pem ./
cp ${WOLFSSL_ROOT_DIR}/certs/client-key.pem ./
cp ${WOLFSSL_ROOT_DIR}/certs/ca-cert.pem ./

echo "wolfSSL test" | openssl pkcs12 -des3 -descert -export -in client-cert.pem -inkey client-key.pem -certfile ca-cert.pem -out test-clientbundle.p12 -password stdin
