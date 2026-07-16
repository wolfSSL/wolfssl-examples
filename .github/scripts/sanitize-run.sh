#!/bin/sh
# Run a curated set of self-contained crypto/pk examples and assert their output.
# MODE=asan expects a wolfSSL built with -fsanitize=address,undefined and rebuilds
# each example the same way; MODE=valgrind expects a normal wolfSSL and runs each
# binary under memcheck; MODE=plain just runs them (the feature-config matrix uses
# this to prove the same examples still produce correct results under a different
# math backend or a leaner build). Self-contained only -- no netns, no server pair.
set -eu

MODE="${1:?usage: sanitize-run.sh <asan|valgrind|plain> <wolfssl-prefix>}"
WOLFSSL="${2:?missing wolfssl prefix}"

export LD_LIBRARY_PATH="$WOLFSSL/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# leak detection is Linux-only; override ASAN_OPTIONS=detect_leaks=0 to run on macOS
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:abort_on_error=1:allocator_may_return_null=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"

case "$MODE" in
    asan)     SAN="-fsanitize=address,undefined -fno-sanitize-recover=all -g"; WRAP="" ;;
    valgrind) SAN=""; WRAP="valgrind --error-exitcode=1 --leak-check=full --errors-for-leak-kinds=definite -q" ;;
    plain)    SAN=""; WRAP="" ;;
    *)        echo "unknown mode: $MODE"; exit 2 ;;
esac

# generated inputs for the file-based examples
printf 'wolfssl-examples analysis input\n' > crypto/aes-modes/in.txt

fail=0
one() {
    dir="$1"; bin="$2"; want="$3"; shift 3
    ( cd "$dir"
      make clean >/dev/null 2>&1 || true
      make "$bin" ${CC:+CC=$CC} WOLFSSL_INSTALL_DIR="$WOLFSSL" \
          CFLAGS="-I$WOLFSSL/include -DWOLFSSL_USE_OPTIONS_H $SAN" \
          LIBS="-L$WOLFSSL/lib -lwolfssl $SAN" >/dev/null
      out=$($WRAP "./$bin" "$@" 2>&1) || { echo "FAIL($MODE) $dir/$bin: nonzero exit"; printf '%s\n' "$out" | tail -5; exit 1; }
      printf '%s' "$out" | grep -qF "$want" || { echo "FAIL($MODE) $dir/$bin: missing '$want'"; exit 1; }
      echo "PASS($MODE) $dir/$bin"
    ) || fail=1
}

one hash                    sha256-hash    "Hash result is"                  input.txt
one crypto/aes-modes        aes-cbc        "Success!"                        in.txt out.bin
one crypto/aes-modes        aes-gcm        "Success!"                        in.txt out.bin
one signature/ecc-sign-verify ecc_sign_verify "Successfully verified signature"
one pk/ed25519              ed25519_verify "Success"

# ecc-verify needs the SECP256K1 Koblitz curve; run it only where the build has it
if grep -q "define HAVE_ECC_KOBLITZ" "$WOLFSSL/include/wolfssl/options.h" 2>/dev/null; then
    one ecc                 ecc-verify     "hash_firmware_verify: 0"
else
    echo "SKIP($MODE) ecc/ecc-verify: build has no HAVE_ECC_KOBLITZ"
fi

exit $fail
