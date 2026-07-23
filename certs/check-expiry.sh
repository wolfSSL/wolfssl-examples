#!/bin/sh
# Walk the committed certificates and CRLs and flag ones that are about to
# expire, so a cert dying becomes a scheduled regeneration instead of a red
# morning. Already expired ones are reported but not failed: several are legacy
# or deliberately stale fixtures (ntru-cert.pem, expired-* used for negative
# tests). Covers PEM and DER X.509 certs, and PEM/DER CRLs (nextUpdate).
set -eu

DIR="${1:-certs}"
WARN_DAYS="${WARN_DAYS:-30}"
FAIL_DAYS="${FAIL_DAYS:-7}"
warn_secs=$((WARN_DAYS * 86400))
fail_secs=$((FAIL_DAYS * 86400))
now=$(date +%s)

rc=0
checked=0

# openssl prints dates like "Jun 10 12:00:00 2027 GMT"; parse with GNU then BSD.
to_epoch() {
    date -d "$1" +%s 2>/dev/null || date -jf "%b %e %T %Y %Z" "$1" +%s 2>/dev/null || echo 0
}

check_cert() {
    f="$1"; form="$2"
    openssl x509 -inform "$form" -noout -in "$f" >/dev/null 2>&1 || return 0
    checked=$((checked + 1))
    if ! openssl x509 -inform "$form" -checkend 0 -noout -in "$f" >/dev/null 2>&1; then
        echo "::notice::already expired (legacy/fixture, not failing): $f"
    elif ! openssl x509 -inform "$form" -checkend "$fail_secs" -noout -in "$f" >/dev/null 2>&1; then
        end=$(openssl x509 -inform "$form" -enddate -noout -in "$f" | cut -d= -f2)
        echo "::error::$f expires within $FAIL_DAYS days ($end) -- regenerate now"
        rc=1
    elif ! openssl x509 -inform "$form" -checkend "$warn_secs" -noout -in "$f" >/dev/null 2>&1; then
        end=$(openssl x509 -inform "$form" -enddate -noout -in "$f" | cut -d= -f2)
        echo "::warning::$f expires within $WARN_DAYS days ($end) -- schedule regeneration"
    fi
}

check_crl() {
    f="$1"; form="$2"
    nu=$(openssl crl -inform "$form" -nextupdate -noout -in "$f" 2>/dev/null | cut -d= -f2)
    [ -n "$nu" ] || return 0
    checked=$((checked + 1))
    secs=$(to_epoch "$nu")
    [ "$secs" -ne 0 ] || return 0
    if [ "$secs" -le "$now" ]; then
        echo "::notice::CRL past nextUpdate (fixture, not failing): $f"
    elif [ "$secs" -le $((now + fail_secs)) ]; then
        echo "::error::$f CRL nextUpdate within $FAIL_DAYS days ($nu) -- regenerate now"
        rc=1
    elif [ "$secs" -le $((now + warn_secs)) ]; then
        echo "::warning::$f CRL nextUpdate within $WARN_DAYS days ($nu) -- schedule regeneration"
    fi
}

for f in $(find "$DIR" -name '*.pem' | sort); do check_cert "$f" PEM; done
for f in $(find "$DIR" -name '*.der' | sort); do check_cert "$f" DER; done
for f in $(find "$DIR" \( -name '*crl*.pem' -o -name '*.crl' \) | sort); do check_crl "$f" PEM; done
for f in $(find "$DIR" -name '*crl*.der' | sort); do check_crl "$f" DER; done

echo "checked $checked cert/CRL file(s); fail threshold ${FAIL_DAYS}d, warn ${WARN_DAYS}d"
exit $rc
