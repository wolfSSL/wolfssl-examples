#!/bin/sh
# Run 3 DTLS multicast peers on the host, let them exchange a few rounds, then
# SIGTERM and assert a peer decrypted another's message. Every peer must share
# DTLS_MCAST_SECRET -- without it each derives a different key and none decrypt.
# Not a manifest entry: mcast-peer is a while(running) loop with no foreground
# driver, and multicast needs a full stack, not the harness loopback-only netns.
set -eu

WOLFSSL="${1:?usage: mcast-run.sh <wolfssl-prefix>}"
export LD_LIBRARY_PATH="$WOLFSSL/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DTLS_MCAST_SECRET=0123456789abcdef0123456789abcdef

cd dtls-mcast
make clean >/dev/null 2>&1 || true
make mcast-peer WOLFSSL_INSTALL_DIR="$WOLFSSL" \
    CFLAGS="-Wall -I$WOLFSSL/include -DWOLFSSL_USE_OPTIONS_H" >/dev/null

./mcast-peer 0 >/tmp/mp0.log 2>&1 & p0=$!
./mcast-peer 1 >/tmp/mp1.log 2>&1 & p1=$!
./mcast-peer 2 >/tmp/mp2.log 2>&1 & p2=$!
sleep 12
kill "$p0" "$p1" "$p2" 2>/dev/null || true
wait "$p0" "$p1" "$p2" 2>/dev/null || true

echo "=== peer log tails ==="
tail -n 4 /tmp/mp0.log /tmp/mp1.log /tmp/mp2.log
hits=$(cat /tmp/mp0.log /tmp/mp1.log /tmp/mp2.log | grep -c "Received from peer" || true)
[ "$hits" -ge 1 ] || { echo "FAIL: no peer received a multicast message"; exit 1; }
echo "PASS: $hits multicast messages received across peers"
