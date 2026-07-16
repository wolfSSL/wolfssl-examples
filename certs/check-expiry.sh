#!/bin/sh
# Walk the committed certificates and flag ones that are about to expire, so a
# cert dying becomes a scheduled regeneration instead of a red morning. Already
# expired certs are reported but not failed: several are legacy or deliberately
# stale fixtures (ntru-cert.pem, expired-* used for negative tests).
set -eu

DIR="${1:-certs}"
WARN_DAYS="${WARN_DAYS:-30}"
FAIL_DAYS="${FAIL_DAYS:-7}"
warn_secs=$((WARN_DAYS * 86400))
fail_secs=$((FAIL_DAYS * 86400))

rc=0
checked=0
for f in $(find "$DIR" -name '*.pem' | sort); do
    # skip PEMs that are not X.509 certs (keys, CSRs, params)
    openssl x509 -noout -in "$f" >/dev/null 2>&1 || continue
    checked=$((checked + 1))

    if ! openssl x509 -checkend 0 -noout -in "$f" >/dev/null 2>&1; then
        echo "::notice::already expired (legacy/fixture, not failing): $f"
    elif ! openssl x509 -checkend "$fail_secs" -noout -in "$f" >/dev/null 2>&1; then
        end=$(openssl x509 -enddate -noout -in "$f" | cut -d= -f2)
        echo "::error::$f expires within $FAIL_DAYS days ($end) -- regenerate now"
        rc=1
    elif ! openssl x509 -checkend "$warn_secs" -noout -in "$f" >/dev/null 2>&1; then
        end=$(openssl x509 -enddate -noout -in "$f" | cut -d= -f2)
        echo "::warning::$f expires within $WARN_DAYS days ($end) -- schedule regeneration"
    fi
done

echo "checked $checked certificate(s); fail threshold ${FAIL_DAYS}d, warn ${WARN_DAYS}d"
exit $rc
