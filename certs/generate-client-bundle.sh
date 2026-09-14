#!/bin/sh
# Rebuild test-clientbundle.p12, the PKCS#12 bundle tls/client-tls-pkcs12.c
# loads (client cert, client key and the CA that signed the server cert).
#
# By default the bundle is built from the PEMs already in this directory,
# so it stays in step with the server-cert.pem the server examples use.
# Set WOLFSSL_ROOT_DIR to a wolfSSL checkout to refresh those PEMs from it
# first. The password is the one the example uses.
#
# Rerun this whenever the PEMs are regenerated: the bundle carries its own
# copies of the certs and expires with them (certs/check-expiry.sh flags
# it 30 days ahead).
set -eu

cd "$(dirname "$0")"
pass="wolfSSL test"
out="test-clientbundle.p12"
now=$(date +%s)

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

# "Sep 14 21:25:29 2027 GMT" -> epoch seconds; GNU date, then BSD date.
to_epoch() {
    date -d "$1" +%s 2>/dev/null \
        || date -jf "%b %e %T %Y %Z" "$1" +%s 2>/dev/null \
        || echo 0
}

# Print one line per cert in a bundle and set end_text/end_secs to the
# earliest notAfter. Fails if the bundle does not open with the password.
describe() {
    certs=$(openssl pkcs12 -in "$1" -nokeys -passin "pass:${pass}" \
        2>/dev/null) || return 1
    n=$(printf '%s\n' "$certs" | grep -c -- '-----BEGIN CERTIFICATE-----')
    end_secs=0
    end_text=""
    i=0
    while [ "$i" -lt "$n" ]; do
        i=$((i + 1))
        cert=$(printf '%s\n' "$certs" | awk -v want="$i" \
            '/-----BEGIN CERTIFICATE-----/{c++} c==want')
        subj=$(printf '%s\n' "$cert" | openssl x509 -noout -subject \
            | sed 's/^subject=//')
        end=$(printf '%s\n' "$cert" | openssl x509 -noout -enddate \
            | sed 's/^notAfter=//')
        secs=$(to_epoch "$end")
        if [ "$end_secs" -eq 0 ] || [ "$secs" -lt "$end_secs" ]; then
            end_secs=$secs
            end_text=$end
        fi
        echo "  cert ${i}: notAfter ${end}  (${subj})"
    done
}

# Says "valid until <date> (N days left)" or "expired <date> (N days ago)".
lifetime() {
    if [ "$end_secs" -gt "$now" ]; then
        days=$(( (end_secs - now) / 86400 ))
        echo "valid until ${end_text} (${days} days left)"
    else
        days=$(( (now - end_secs) / 86400 ))
        echo "expired ${end_text} (${days} days ago)"
    fi
}

if [ -f "$out" ]; then
    if describe "$out" >/dev/null; then
        echo "previous ${out}: $(lifetime)"
    else
        echo "previous ${out}: does not open with the example's password"
    fi
else
    echo "no previous ${out}"
fi

if [ -n "${WOLFSSL_ROOT_DIR:-}" ]; then
    for f in client-cert.pem client-key.pem ca-cert.pem; do
        src="${WOLFSSL_ROOT_DIR}/certs/${f}"
        [ -r "$src" ] || fail "cannot read ${src}"
        cp "$src" ./
        echo "copied ${f} from ${WOLFSSL_ROOT_DIR}"
    done
fi
for f in client-cert.pem client-key.pem ca-cert.pem; do
    [ -r "$f" ] || fail "missing ${f} in $(pwd)"
done

# Explicit algorithms so the output does not drift with the OpenSSL
# version: 3DES for the certs and AES-256 (PBES2) for the key, SHA-256 MAC,
# which is what the committed bundle has always used and wolfSSL parses.
echo "$pass" | openssl pkcs12 -export -in client-cert.pem \
    -inkey client-key.pem -certfile ca-cert.pem \
    -certpbe PBE-SHA1-3DES -keypbe AES-256-CBC -macalg sha256 \
    -out "$out" -password stdin || fail "openssl pkcs12 -export failed"

# Read it back the way the example will, and report what is inside.
describe "$out" \
    || fail "${out} was written but does not open with the example's password"
[ "$n" -eq 2 ] || fail "${out} holds ${n} certificate(s), expected client + CA"
if ! openssl pkcs12 -in "$out" -nocerts -nodes -passin "pass:${pass}" \
        2>/dev/null | openssl pkey -noout 2>/dev/null; then
    fail "${out} holds no readable private key"
fi
[ "$end_secs" -gt "$now" ] \
    || fail "new ${out} is already expired ($(lifetime)); refresh the PEMs"

echo "OK: wrote $(pwd)/${out} ($(wc -c < "$out") bytes, ${n} certs, key)"
echo "new ${out}: $(lifetime)"
