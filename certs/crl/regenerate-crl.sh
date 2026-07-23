#!/bin/sh
# Regenerate crl.pem, which certmanager/certverify loads.
#
# The previous one expired 2025-09-11 and silently broke every CRL example until
# CI started checking exit codes. Re-run this when nextUpdate gets close:
#   openssl crl -in crl.pem -noout -nextupdate

set -e
cd "$(dirname "$0")/../.."

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# certs/server-revoked-cert.pem is serial 02; keep its original revocation date
# so the CRL keeps saying the same thing.
EXPIRY=$(openssl x509 -in certs/server-revoked-cert.pem -noout -enddate | cut -d= -f2)
EXPIRY_Z=$(python3 -c "
import datetime
print(datetime.datetime.strptime('''$EXPIRY''', '%b %d %H:%M:%S %Y %Z').strftime('%y%m%d%H%M%S') + 'Z')")
SUBJECT=$(openssl x509 -in certs/server-revoked-cert.pem -noout -subject -nameopt compat | sed 's/^subject=//')

printf 'R\t%s\t221216211750Z\t02\tunknown\t%s\n' "$EXPIRY_Z" "$SUBJECT" > "$WORK/index.txt"
# RFC 5280 wants a monotonically increasing CRL number: seed from the CRL being
# replaced so the new one supersedes it instead of colliding.
# openssl prints crlNumber=0x1000 on new versions and crlNumber=1000 on old ones
CURNUM=$(openssl crl -in certs/crl/crl.pem -noout -crlnumber 2>/dev/null | cut -d= -f2)
CURNUM=${CURNUM#0x}
printf '%X\n' "$(( 0x${CURNUM:-0FFF} + 1 ))" > "$WORK/crlnumber"

cat > "$WORK/ca.cnf" <<EOF
[ ca ]
default_ca = CA_default
[ CA_default ]
database         = $WORK/index.txt
crlnumber        = $WORK/crlnumber
default_md       = sha256
default_crl_days = 3650
EOF

openssl ca -config "$WORK/ca.cnf" -gencrl \
    -keyfile certs/ca-key.pem -cert certs/ca-cert.pem \
    -out certs/crl/crl.pem

openssl crl -in certs/crl/crl.pem -CAfile certs/ca-cert.pem -noout
openssl crl -in certs/crl/crl.pem -noout -nextupdate
