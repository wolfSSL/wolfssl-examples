#!/bin/sh
# Build two self-contained crypto examples for a cross target and run them --
# natively for -m32, under qemu-user otherwise. s390x is big-endian, so this is
# the only tier that would catch an endianness bug in the bignum/ECC paths.
set -eu

: "${CROSS_CC:?set CROSS_CC}"
: "${WOLFSSL:?set WOLFSSL install prefix}"
CFLAGS_EXTRA="${CROSS_CFLAGS:-}"
WRAP="${CROSS_WRAP:-}"

fail=0
one() {
    src="$1"; want="$2"; bin="/tmp/$(basename "$src" .c)-cross"
    # unquoted on purpose: CROSS_CC may carry flags and CFLAGS_EXTRA is a word list
    # shellcheck disable=SC2086
    $CROSS_CC $CFLAGS_EXTRA "$src" -I"$WOLFSSL/include" -DWOLFSSL_USE_OPTIONS_H \
        -static -o "$bin" "$WOLFSSL/lib/libwolfssl.a" -lm \
        || { echo "FAIL build $src"; fail=1; return; }
    # shellcheck disable=SC2086
    out=$($WRAP "$bin" 2>&1) || { echo "FAIL run $src"; printf '%s\n' "$out" | tail -5; fail=1; return; }
    printf '%s' "$out" | grep -qF "$want" || { echo "FAIL assert $src: want '$want'"; fail=1; return; }
    echo "PASS $(basename "$src")"
}

one signature/ecc-sign-verify/ecc_sign_verify.c "Successfully verified signature"
one pk/ed25519/ed25519_verify.c                 "Success"

exit $fail
