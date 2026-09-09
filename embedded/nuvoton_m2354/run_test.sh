#!/bin/bash
#
# Build, flash and capture the wolfCrypt test/benchmark on a NuMaker-M2354.
#
# Copyright (C) 2006-2026 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# Usage:
#   ./run_test.sh --bsp /path/to/M2354BSP [--wolfssl PATH] [--seconds 900]
#                 [--no-build]
#
# Set EXPECT_WORLD to the banner's TrustZone line - "secure world, direct BSP
# calls" or "non-secure, hardware through NSC veneers" - to have the run fail
# rather than report results from an image you did not mean to test.
#
# The board has one Nu-Link2-Me carrying both SWD and the VCOM console, so it
# is a singleton on a shared bench. This script takes a lock for the whole
# flash-and-capture cycle and always releases it, including on failure.

set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
BSP=""
WOLFSSL_ARG=""
SECONDS_CAPTURE=900
DO_BUILD=1
SESSION="${CLAUDE_SESSION_NAME:-$(whoami)@$(hostname)}"

LOCKDIR=/tmp/claude-bench-locks
LOCK="$LOCKDIR/numaker-m2354.lock"

while [ $# -gt 0 ]; do
    case "$1" in
        --bsp)      BSP="$2"; shift 2 ;;
        --wolfssl)  WOLFSSL_ARG="--wolfssl $2"; shift 2 ;;
        --seconds)  SECONDS_CAPTURE="$2"; shift 2 ;;
        --no-build) DO_BUILD=0; shift ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

if [ "$DO_BUILD" -eq 1 ] && [ -z "$BSP" ]; then
    echo "give the BSP location with --bsp /path/to/M2354BSP" >&2
    exit 1
fi

# ---- find the probe and its VCOM ------------------------------------------
# Nu-Link2-Me enumerates as Nuvoton (0416) with its own firmware, or as
# ARM mbed DAPLink (0d28) after loading NuLink2_DAPLink.bin. pyocd needs the
# DAPLink flavour; Nuvoton's OpenOCD fork needs the other.
find_probe() {
    local d v pr
    for d in /sys/bus/usb/devices/*-*; do
        [ -f "$d/idVendor" ] || continue
        v=$(cat "$d/idVendor")
        if [ "$v" = "0416" ] || [ "$v" = "0d28" ]; then
            pr=$(cat "$d/product" 2>/dev/null)
            echo "$(basename "$d") $v:$(cat "$d/idProduct") $pr"
            return 0
        fi
    done
    return 1
}

find_vcom() {
    local d dev v
    for d in /sys/class/tty/ttyACM*; do
        [ -e "$d" ] || continue
        dev=/dev/$(basename "$d")
        v=$(udevadm info -q property -n "$dev" 2>/dev/null | \
            sed -n 's/^ID_VENDOR_ID=//p')
        if [ "$v" = "0416" ] || [ "$v" = "0d28" ]; then
            echo "$dev"
            return 0
        fi
    done
    return 1
}

PROBE=$(find_probe) || {
    echo "No Nu-Link2-Me found (looked for USB VID 0416 or 0d28)." >&2
    echo "Check that the USB cable is in the Nu-Link2-Me connector, that the" >&2
    echo "debugger daughterboard is attached, and that the board has power." >&2
    exit 1
}
echo "probe: $PROBE"

VCOM=$(find_vcom) || {
    echo "Probe found but it presents no CDC-ACM console." >&2
    exit 1
}
echo "console: $VCOM"

# ---- build ----------------------------------------------------------------
if [ "$DO_BUILD" -eq 1 ]; then
    # shellcheck disable=SC2086
    "$HERE/build.sh" --bsp "$BSP" $WOLFSSL_ARG
fi

ELF="$HERE/build/wolfcrypt-m2354.elf"
test -f "$ELF" || { echo "no $ELF, build first" >&2; exit 1; }

# ---- take the bench lock --------------------------------------------------
mkdir -p "$LOCKDIR"
if ! ( set -o noclobber
       echo "$(date -Is) $SESSION wolfcrypt_test flash+capture" > "$LOCK"
     ) 2>/dev/null; then
    echo "NuMaker-M2354 is in use: $(cat "$LOCK")" >&2
    echo "Wait for it to be released, or coordinate with the holder." >&2
    exit 1
fi
echo "lock: $(cat "$LOCK")"

RELEASED=0
cleanup() {
    if [ "$RELEASED" -eq 0 ]; then
        uart-monitor reclaim "$VCOM" >/dev/null 2>&1 || true
        rm -f "$LOCK"
        RELEASED=1
        echo "lock released"
    fi
}
trap cleanup EXIT INT TERM

# ---- flash ----------------------------------------------------------------
# Take the console away from the shared daemon for the whole cycle, and give it
# back in the trap. Reading the daemon's log instead looks tidier and works
# right up until it does not: on long runs the daemon stops capturing this port
# while still reporting it as monitored, and the log simply stops growing with
# no error anywhere. That is indistinguishable from a firmware hang, and it
# cost a full afternoon of chasing one that did not exist. Owning the tty for
# the duration removes the failure mode.
#
# The important part is that the daemon stays yielded until the capture is
# finished. Reclaiming before reading puts two readers on one tty and each
# steals bytes from the other, which shreds the output instead of losing it.
uart-monitor yield "$VCOM" >/dev/null 2>&1 || true

echo "=== flashing ==="
# This bench has a dozen-plus debug probes attached. Without -u, pyocd prompts
# for which one to use and then dies on EOF in a non-interactive run.
# Match the id by its shape rather than by column position: pyocd's table has
# a trailing target column that is present or absent depending on whether the
# probe's part could be identified, which moves the fields around.
PROBE_UID=$(pyocd list 2>/dev/null | grep -i 'nu-link2' | \
            grep -oE '[0-9A-Fa-f]{16,}' | head -1)
test -n "$PROBE_UID" || { echo "could not determine the Nu-Link2 probe id" >&2; exit 1; }
echo "probe uid: $PROBE_UID"
# Sector erase, not chip erase. A chip erase also clears the user
# configuration area, which on a TrustZone-provisioned part holds NSCBA - the
# secure/non-secure flash boundary. Erasing that silently un-provisions the
# board for anyone else sharing it, and the damage is invisible until their
# next secure build behaves oddly. Sector erase touches only what is being
# programmed, and is faster besides.
pyocd flash --target m2354kjfae -u "$PROBE_UID" --erase sector "$ELF"

# ---- capture --------------------------------------------------------------
mkdir -p "$HERE/logs"
OUT="$HERE/logs/console-$(date +%Y%m%d-%H%M%S).log"
echo "=== capturing to $OUT (up to $SECONDS_CAPTURE s) ==="

stty -F "$VCOM" 115200 raw -echo -echoe -echok -crtscts 2>/dev/null || true

# Whatever the previous image was still emitting, so it cannot be mistaken for
# this run: the two are hard to tell apart once they are in one file.
timeout 1 cat "$VCOM" >/dev/null 2>&1 || true

# Read first, reset second. Resetting before the reader is attached loses the
# banner and the first tests, which is what makes a log start mid-run.
#
# Read with a plain cat and poll the file, rather than stopping the reader on a
# sentinel line: a sed -u pattern quit here gave up on the tty a couple of
# minutes in, which truncated the log at whatever test was running and read as
# a hang. cat holds the port for as long as it is given.
cat "$VCOM" > "$OUT" 2>/dev/null &
CAP_PID=$!

sleep 1
pyocd reset -t m2354kjfae -u "$PROBE_UID" >/dev/null 2>&1 || true

# Stop as soon as the application prints its last line. The benchmark's public
# key rows take minutes each on this part, so the window has to be generous and
# waiting it out every time is most of the cycle.
ELAPSED=0
while [ "$ELAPSED" -lt "$SECONDS_CAPTURE" ]; do
    sleep 5
    ELAPSED=$((ELAPSED + 5))
    grep -q "benchmark_test returned" "$OUT" 2>/dev/null && break
done
kill "$CAP_PID" 2>/dev/null || true
wait "$CAP_PID" 2>/dev/null || true

cat "$OUT"

echo
echo "=== summary ==="
grep -E "returned|Test complete|FAIL|failed|hard fault" "$OUT" | tail -20 || true

# Which image produced these results matters, and the application says so in
# its banner. Read it rather than assuming: a partial reflash - a TrustZone
# secure half overwritten by a single-world image, say - leaves a board that
# boots something other than what was just built, and every line below it then
# describes the wrong binary. Treat a missing or unexpected banner as a failed
# run, not a note, because the results are not attributable without it.
if ! grep -q "wolfCrypt on NuMaker" "$OUT"; then
    echo "ERROR: no banner - the capture attached after the reset, or the" >&2
    echo "       board is running something other than this build. The" >&2
    echo "       results above are not attributable; rerun." >&2
    exit 1
fi

# The console sends CRLF, so strip the carriage return or every comparison
# below fails against a string that looks identical when printed.
WORLD=$(sed -n 's/^TrustZone: //p' "$OUT" | head -1 | tr -d '\r')
echo "image reported: TrustZone: ${WORLD:-unknown}"
if [ -n "$EXPECT_WORLD" ] && [ "$WORLD" != "$EXPECT_WORLD" ]; then
    echo "ERROR: expected \"$EXPECT_WORLD\" but the board reported" >&2
    echo "       \"$WORLD\" - wrong image on the part." >&2
    exit 1
fi
# Check the results, not just that the stage ran. A stage that printed
# "returned 1" is a failed run, and a stage that never printed its result at
# all is a truncated capture - reporting either as a pass is worse than
# reporting nothing.
RUN_FAIL=0

check_stage() {
    # $1 label as printed, $2 1 if the stage is required
    line=$(grep -E "^$1 returned " "$OUT" | tail -1 | tr -d '\r')
    if [ -z "$line" ]; then
        if [ "$2" -eq 1 ]; then
            echo "ERROR: $1 never reported a result - capture truncated?" >&2
            RUN_FAIL=1
        else
            echo "NOTE: $1 did not finish inside the window"
        fi
        return
    fi
    rc=${line##* }
    if [ "$rc" != "0" ]; then
        echo "ERROR: $1 returned $rc" >&2
        RUN_FAIL=1
    else
        echo "$1: passed"
    fi
}

# Count only what this boot printed. The capture attaches before the reset, so
# a log routinely opens with the tail of the previous image's output, and a
# plain grep over the file adds two boots together - which is how a wrong test
# count once reached a PR description.
BANNER_LINE=$(grep -n "wolfCrypt on NuMaker" "$OUT" | head -1 | cut -d: -f1)
TESTS=$(tail -n +"${BANNER_LINE:-1}" "$OUT" | grep -c "test passed")
echo "tests passed this boot: $TESTS"

check_stage keystore_test 1
check_stage wolfcrypt_test 1
check_stage benchmark_test 0

# The suite prints its own per-test failures; catch them even if the summary
# return value was somehow absent.
if grep -qE "test failed|error = " "$OUT"; then
    echo "ERROR: the log contains a failed test" >&2
    RUN_FAIL=1
fi

echo "full log: $OUT"

if [ "$RUN_FAIL" -ne 0 ]; then
    exit 1
fi
