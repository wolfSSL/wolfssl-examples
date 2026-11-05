#!/bin/sh

# Run client-tls-mbedtls against the check servers. Each case gets a new
# server because they exit after a failed handshake. WRAP prefixes the client.
# BIN_DIR is where they are built, by default the current directory.

BIN_DIR=${BIN_DIR:-.}
FAILS=0

# Retry while the server is not listening yet. A probe connection would end
# the server, since it exits when a handshake fails.
run_client() {
    i=0
    while :; do
        out=$($WRAP "$BIN_DIR/client-tls-mbedtls" $opts 127.0.0.1 2>&1)
        rc=$?
        [ "$out" != "ERROR: failed to connect" ] && return
        i=$((i + 1))
        [ $i -ge 50 ] && return
        sleep 0.1
    done
}

# name|server|client options|expected tls_get() return
while IFS='|' read -r name server opts expect; do
    [ -z "$name" ] && continue
    "$BIN_DIR/$server" >/dev/null 2>&1 &
    srv=$!
    run_client
    want=1
    [ "$expect" -eq 0 ] && want=0
    if [ "$out" = "tls_get() returned $expect" ] && [ $rc -eq $want ]; then
        echo "PASS: $name"
    else
        echo "FAIL: $name: $out"
        FAILS=$((FAILS + 1))
    fi
    kill $srv 2>/dev/null
    wait $srv 2>/dev/null
done <<EOF
TLS 1.3|check-server-tls13||0
TLS 1.2|check-server-tls12||0
TLS 1.3 non-blocking|check-server-tls13|-N|0
TLS 1.2 non-blocking|check-server-tls12|-N|0
DER CA file|check-server-tls13|-A ../certs/ca-cert.der|0
wrong host, VERIFY_FAILED|check-server-tls13|-S wrong.example.org|-9984
wrong CA, VERIFY_FAILED|check-server-tls13|-A ../certs/ca-ecc-cert.pem|-9984
no CA file, X509_FILE_IO_ERROR|check-server-tls13|-A ../certs/missing.pem|-10496
not a certificate, X509_INVALID_FORMAT|check-server-tls13|-A README.md|-8576
IP address host, SSL_BAD_INPUT_DATA|check-server-tls13|-S 127.0.0.1|-135
EOF

if [ $FAILS -ne 0 ]; then
    echo "$FAILS failed"
    exit 1
fi
echo "PASS: mbedtls-compat checks"
