#!/bin/bash
#
# Build and run every signature example for each math/arch configuration.
#
# Usage: ./verify_signature.sh [WOLFROOT] [LOGDIR]
#   WOLFROOT : wolfSSL source directory (default: ../../../wolfssl)
#   LOGDIR   : directory for build/run logs (default: ./verify_logs)
#
# A one-line result per example/config/target is printed and saved to
# LOGDIR/summary.txt. Combinations not supported by wolfSSL are shown as N/A.
# Exit status is non-zero if any build or run fails.
#
# NOTE: every Makefile target runs "clean", which deletes
#       WOLFROOT/wolfcrypt/src/*.o. Use a separate wolfSSL copy if needed.

SIGDIR="$(cd "$(dirname "$0")" && pwd)"
WOLFROOT="$(realpath "${1:-$SIGDIR/../../../wolfssl}")"
LOGDIR="$(realpath -m "${2:-$SIGDIR/verify_logs}")"

if [ ! -f "$WOLFROOT/wolfcrypt/src/ecc.c" ]; then
    echo "wolfSSL source not found: $WOLFROOT" >&2
    exit 1
fi
mkdir -p "$LOGDIR"
SUMMARY="$LOGDIR/summary.txt"
: > "$SUMMARY"

# name|math|arch
CONFIGS=("default||" "tfm|math=tfm|" "sp_c32|math=sp|arch=c32" \
         "sp_c64|math=sp|arch=c64")
case "$(uname -m)" in
    x86_64)        CONFIGS+=("sp_x64|math=sp|arch=x64") ;;
    aarch64|arm64) CONFIGS+=("sp_arm64|math=sp|arch=arm64") ;;
esac

# dir -> "make_target:binary ..."
declare -A TARGETS
TARGETS[ecc_sign_verify]="ecc_sign_verify:ecc_sign_verify bench:ecc_sign_verify_bench mem:ecc_sign_verify_mem"
TARGETS[ecc_vfy_only]="ecc_verify:ecc_verify bench:ecc_verify_bench mem:ecc_verify_mem"
TARGETS[ecc_vfy_only_nonblock]="ecc_verify_nonblock:ecc_verify_nonblock mem:ecc_verify_nonblock_mem"
TARGETS[rsa_sign_verify]="rsa_sign_verify:rsa_sign_verify bench:rsa_sign_verify_bench mem:rsa_sign_verify_mem nonblock:rsa_sign_verify_nonblock"
TARGETS[rsa_vfy_only]="verify:verify bench:verify_bench mem:verify_mem"
TARGETS[rsa_vfy_only_nonblock]="verify_nonblock:verify_nonblock mem:verify_mem_nonblock"
DIRS="ecc_sign_verify ecc_vfy_only ecc_vfy_only_nonblock rsa_sign_verify rsa_vfy_only rsa_vfy_only_nonblock"

# Combinations wolfSSL does not support. Key: <dir>__<config>__<target>
declare -A NOT_SUPPORTED
NOT_SUPPORTED[ecc_vfy_only_nonblock__tfm__ecc_verify_nonblock]="ECC non-blocking is SP only"
NOT_SUPPORTED[ecc_vfy_only_nonblock__tfm__mem]="ECC non-blocking is SP only"
for c in default sp_c32 sp_c64 sp_x64 sp_arm64; do
    NOT_SUPPORTED[rsa_sign_verify__${c}__nonblock]="RSA non-blocking is TFM only"
done

fail=0
n_ok=0
n_fail=0
n_na=0
failed=""
for d in $DIRS; do
    for c in "${CONFIGS[@]}"; do
        IFS='|' read -r cname m a <<< "$c"
        for t in ${TARGETS[$d]}; do
            tgt="${t%%:*}"; bin="${t##*:}"
            key="${d}__${cname}__${tgt}"
            log="$LOGDIR/$key.log"
            if [ -n "${NOT_SUPPORTED[$key]}" ]; then
                build="N/A"; run="N/A (${NOT_SUPPORTED[$key]})"
            elif make -C "$SIGDIR/$d" WOLFROOT="$WOLFROOT" $m $a \
                    EX_CFLAGS="-DBENCH_TIME_SEC=0.2" "$tgt" > "$log" 2>&1; then
                build=OK
                if timeout 300 "$SIGDIR/$d/$bin" >> "$log" 2>&1; then
                    run=OK
                else
                    run="FAIL($?)"; fail=1
                fi
            else
                build=FAIL; run=-; fail=1
            fi
            printf "%-22s %-8s %-20s build=%-4s run=%s\n" \
                "$d" "$cname" "$tgt" "$build" "$run" | tee -a "$SUMMARY"
            case "$build/$run" in
                N/A/*) n_na=$((n_na + 1)) ;;
                OK/OK) n_ok=$((n_ok + 1)) ;;
                *)     n_fail=$((n_fail + 1)); failed+="  $key ($log)"$'\n' ;;
            esac
        done
    done
    make -C "$SIGDIR/$d" WOLFROOT="$WOLFROOT" clean > /dev/null 2>&1
done

{
    echo ""
    echo "==== Summary ===="
    echo "Total: $((n_ok + n_fail + n_na))  OK: $n_ok  FAIL: $n_fail  N/A: $n_na"
    if [ -n "$failed" ]; then
        echo "Failed:"
        printf "%s" "$failed"
    fi
    echo "Logs: $LOGDIR"
} | tee -a "$SUMMARY"
exit $fail
