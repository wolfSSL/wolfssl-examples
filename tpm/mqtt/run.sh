#!/usr/bin/bash

WOLFSSL=../../../wolfssl
WOLFTPM=../../../wolftpm

set -e
set -x

TPM_SERVER_REDIR="&> /dev/null"
MOSQUITTO_REDIR=""
MOSQUITTO_PUB_REDIR="&> /dev/null"
CLIENT_REDIR=""
use_tmux=false
#use_tmux=true

if ${use_tmux} && which tmux > /dev/null ; then
    echo "Using tmux"
    TMUX_SESSION=mqtt-tpm
    tmux new-session -d -s "${TMUX_SESSION}"

    TPM_SERVER_REDIR="2>&1 | tmux split-window -dI -p 10 -t \"${TMUX_SESSION}:\""
    MOSQUITTO_REDIR="2>&1 | tmux split-window -dI  -p 10 -t \"${TMUX_SESSION}:\""
    CLIENT_REDIR="2>&1 | tmux split-window -dI -b -h -f -e \"LD_LIBRARY_PATH=$PWD/inst/lib\" -p 33 -t \"${TMUX_SESSION}:\""
    MOSQUITTO_PUB_REDIR="2>&1 | tmux split-window -dI -h -p 50 -t \"${TMUX_SESSION}:\""
fi

# start simulator (clean state)
eval ${WOLFTPM}/ibmswtpm2/src/tpm_server -rm ${TPM_SERVER_REDIR} &


#generate TPM key, csr, and certs
pushd ${WOLFTPM}
# clean certs
./certs/certreq.sh clean
# generate key
./examples/keygen/keygen
# generate csr
./examples/csr/csr
# sign csr
./certs/certreq.sh
popd

# copy ca cert, wrapped blob, and client cert locally
src_files=(                                     \
  ${WOLFSSL}/certs/ca-cert.pem                  \
  ${WOLFSSL}/certs/server-cert.pem              \
  ${WOLFSSL}/certs/server-key.pem               \
  ${WOLFSSL}/certs/client-ca.pem                \
  ${WOLFSSL}/certs/client-cert.pem              \
  ${WOLFSSL}/certs/client-key.pem               \
  ${WOLFTPM}/certs/ca-rsa-cert.pem              \
  ${WOLFTPM}/rsa_test_blob.raw                  \
  ${WOLFTPM}/certs/client-rsa-cert.pem          \
  )

mkdir -p tmp_certs_and_keys/
cp -v ${src_files[*]} tmp_certs_and_keys/

# concatenate CA, TPM CA, and client CA to be used by mosquitto
cat tmp_certs_and_keys/ca-cert.pem \
    tmp_certs_and_keys/ca-rsa-cert.pem \
    tmp_certs_and_keys/client-ca.pem \
    > tmp_certs_and_keys/ca-list.pem

# start server
eval mosquitto -v -c mosquitto.conf ${MOSQUITTO_REDIR} &
mosq_pid=$!

sleep 5
#start client
export LD_LIBRARY_PATH=$PWD/inst/lib
eval ./mqtt_tpm_simple ${CLIENT_REDIR} &

sleep 5
#publish message
eval mosquitto_pub -d -h 127.0.0.1 -p 18883 \
     --cafile tmp_certs_and_keys/ca-cert.pem \
     --cert tmp_certs_and_keys/client-cert.pem \
     --key tmp_certs_and_keys/client-key.pem \
     -t wolfMQTT/example/testTopic -m "Hola!" ${MOSQUITTO_PUB_REDIR}

sleep 5
#publish message to exit
eval mosquitto_pub -d -h 127.0.0.1 -p 18883 \
     --cafile tmp_certs_and_keys/ca-cert.pem \
     --cert tmp_certs_and_keys/client-cert.pem \
     --key tmp_certs_and_keys/client-key.pem \
     -t wolfMQTT/example/testTopic -m exit ${MOSQUITTO_PUB_REDIR}


sleep 5

pkill -9 -x -u $EUID tpm_server
pkill -9 -x -u $EUID mosquitto
