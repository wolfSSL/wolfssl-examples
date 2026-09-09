#!/bin/bash
#
# Build the wolfCrypt test and benchmark application for the NuMaker-M2354.
#
# Copyright (C) 2006-2026 wolfSSL Inc.
#
# This file is part of wolfSSL.
#
# Usage:
#   ./build.sh --bsp /path/to/M2354BSP [--wolfssl /path/to/wolfssl]
#   ./build.sh --bsp /path/to/M2354BSP --tz     TrustZone compile check
#
# wolfSSL defaults to ../../../wolfssl, or set WOLFSSL_ROOT. Get the BSP with:
#   git clone --depth 1 https://github.com/OpenNuvoton/M2354BSP
#
# The secure build links a complete .elf you can flash. The --tz build is a
# compile check of both halves of the TrustZone split: the port sources as the
# non-secure image sees them, and the secure veneers with -mcmse. It does not
# link, because a working TrustZone application also needs a partition layout
# and a secure bootstrap that belong to your project rather than to wolfSSL.

set -e

CC=${CC:-arm-none-eabi-gcc}
OBJCOPY=${OBJCOPY:-arm-none-eabi-objcopy}
SIZE=${SIZE:-arm-none-eabi-size}
NM=${NM:-arm-none-eabi-nm}

BSP=""
TZ=0
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="${WOLFSSL_ROOT:-$HERE/../../../wolfssl}"
OUT="$HERE/build"

while [ $# -gt 0 ]; do
    case "$1" in
        --bsp)     BSP="$2"; shift 2 ;;
        --wolfssl) ROOT="$2"; shift 2 ;;
        --tz)      TZ=1; shift ;;
        --out)     OUT="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

if [ ! -f "$ROOT/wolfcrypt/src/port/nuvoton/nuvoton_hw.c" ]; then
    echo "no wolfSSL with the Nuvoton port at: $ROOT" >&2
    echo "point at one with --wolfssl PATH or WOLFSSL_ROOT." >&2
    exit 1
fi
ROOT="$(cd "$ROOT" && pwd)"

if [ -z "$BSP" ]; then
    echo "give the BSP location with --bsp /path/to/M2354BSP" >&2
    exit 1
fi

DEV="$BSP/Library/Device/Nuvoton/M2354"
STD="$BSP/Library/StdDriver"

for d in "$DEV/Include" "$STD/inc" "$BSP/Library/CMSIS/Include"; do
    if [ ! -d "$d" ]; then
        echo "not a M2354BSP checkout: $d is missing" >&2
        exit 1
    fi
done

# Clean only the object directory this invocation will use, not the whole
# output tree. The two legs share $OUT, so wiping it means a --tz run deletes
# the .elf the secure run just produced - which you discover later when
# something wants it for symbol resolution and it is gone.
if [ "$TZ" -eq 0 ]; then
    rm -rf "$OUT/obj"
else
    rm -rf "$OUT/obj-ns" "$OUT/tz"
fi
mkdir -p "$OUT"

INC="-I$ROOT -I$HERE -I$DEV/Include -I$STD/inc -I$BSP/Library/CMSIS/Include"

CFLAGS="-mcpu=cortex-m23 -mthumb -Os -ffunction-sections -fdata-sections"
CFLAGS="$CFLAGS -Wall -Wextra -Werror -DWOLFSSL_USER_SETTINGS $INC"

# wolfCrypt, minus the two files that are #included into others rather than
# compiled on their own.
WC_SRC=""
for f in "$ROOT"/wolfcrypt/src/*.c; do
    case "$(basename "$f")" in
        misc.c|evp.c) continue ;;
    esac
    WC_SRC="$WC_SRC $f"
done
WC_SRC="$WC_SRC $ROOT/wolfcrypt/test/test.c $ROOT/wolfcrypt/benchmark/benchmark.c"

PORT_SRC="$ROOT/wolfcrypt/src/port/nuvoton/nuvoton_hw.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_cryptocb.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_cb_rng.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_cb_hash.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_cb_cipher.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_cb_pk.c
          $ROOT/wolfcrypt/src/port/nuvoton/nuvoton_key.c"

BSP_SRC="$DEV/Source/system_M2354.c
         $STD/src/clk.c
         $STD/src/sys.c
         $STD/src/uart.c
         $STD/src/crypto.c
         $STD/src/keystore.c
         $STD/src/rng.c
         $STD/src/fmc.c"

# The BSP is vendor code and does not build clean under our warning set (its
# RSA driver compares a pointer against NULL as an integer), so it gets the
# same flags without -Werror.
BSPFLAGS="-mcpu=cortex-m23 -mthumb -Os -ffunction-sections -fdata-sections"
BSPFLAGS="$BSPFLAGS -DWOLFSSL_USER_SETTINGS $INC"

compile() {
    # compile <output dir> <compiler flags> <sources...>
    local dir="$1"; shift
    local flags="$1"; shift
    local src obj

    mkdir -p "$dir"
    for src in "$@"; do
        obj="$dir/$(echo "$src" | md5sum | cut -c1-12)_$(basename "$src" | sed 's/\.[cS]$//').o"
        # shellcheck disable=SC2086
        $CC $flags -c "$src" -o "$obj" || exit 1
        echo "$obj"
    done
}

if [ "$TZ" -eq 0 ]; then
    echo "=== secure world build ==="

    # shellcheck disable=SC2086
    OBJS=$(compile "$OUT/obj" "$CFLAGS -DWOLFSSL_NUVOTON_SECURE" \
        $WC_SRC $PORT_SRC "$HERE/app.c" "$HERE/keystore_test.c") || exit 1
    # shellcheck disable=SC2086
    OBJS="$OBJS $(compile "$OUT/obj" "$BSPFLAGS" $BSP_SRC)" || exit 1

    # The BSP startup is assembly and the retarget file wants the BSP's own
    # warning level, so neither goes through -Werror.
    # startup_M2354.S reserves the heap with an #ifndef-guarded Heap_Size, so
    # it can be raised from here. The BSP default is 10 KB, which is thin once
    # wolfcrypt_test wants a 2 KB RSA working buffer plus hash accumulation.
    # This is only safe because app.c enables the SRAM1/SRAM2 clocks: a heap
    # reaching past 0x20008000 with those gated off would take writes and
    # discard them silently. 32 KB is ample: the hash path streams through the
    # engine with a fixed ~350 bytes of state per context, so heap use no
    # longer scales with the message being hashed.
    # Stack_Size is #ifndef-guarded in startup_M2354.S too. The BSP default is
    # 12 KB, which software ECC overruns: gcc_arm.ld puts the stack at the
    # BOTTOM of RAM with __StackLimit at 0x20000000, so an overflow runs off
    # the start of SRAM and the fault handler has no stack left to report it.
    # The part simply goes quiet, which is indistinguishable from a hang.
    $CC -mcpu=cortex-m23 -mthumb $INC -DHeap_Size=${HEAP_SIZE:-0x00010000} \
        -DStack_Size=${STACK_SIZE:-0x00008000} \
        -c "$DEV/Source/GCC/startup_M2354.S" -o "$OUT/obj/startup.o"
    $CC -mcpu=cortex-m23 -mthumb -Os $INC -c "$DEV/Source/GCC/_syscalls.c" \
        -o "$OUT/obj/syscalls.o"

    # shellcheck disable=SC2086
    $CC -mcpu=cortex-m23 -mthumb -T"$DEV/Source/GCC/gcc_arm.ld" \
        -Wl,--gc-sections -Wl,-Map="$OUT/wolfcrypt-m2354.map" \
        --specs=nano.specs --specs=nosys.specs -u _printf_float \
        $OBJS "$OUT/obj/startup.o" "$OUT/obj/syscalls.o" \
        -o "$OUT/wolfcrypt-m2354.elf" -lm

    $OBJCOPY -O binary "$OUT/wolfcrypt-m2354.elf" "$OUT/wolfcrypt-m2354.bin"
    $SIZE "$OUT/wolfcrypt-m2354.elf"

    # VTOR alignment. The low bits of VTOR are RES0, and how many depends on
    # the exception count: the M2354 implements 132 (16 system + 116 external),
    # so the table is 528 bytes and VTOR has to be 1024-aligned - not the 128
    # or 256 a smaller part would need. Linking at 0x00000000 satisfies that by
    # construction, but a misaligned table does not fail at boot: do_boot-style
    # handoff code reads the initial SP and PC out of the image directly, so it
    # starts, prints and runs, and then the first exception of any kind vectors
    # through the wrong address. Check it here rather than discover it that way.
    VEC=$($NM "$OUT/wolfcrypt-m2354.elf" | awk '$3 == "__Vectors" {print $1}')
    VEC_END=$($NM "$OUT/wolfcrypt-m2354.elf" | awk '$3 == "__Vectors_End" {print $1}')
    if [ -n "$VEC" ]; then
        VEC_D=$((0x$VEC))
        if [ $((VEC_D % 1024)) -ne 0 ]; then
            echo "vector table at 0x$VEC is not 1024-aligned" >&2
            exit 1
        fi
        if [ -n "$VEC_END" ]; then
            echo "vectors: 0x$VEC, $(( 0x$VEC_END - VEC_D )) bytes, 1024-aligned"
        fi
    fi

    echo
    echo "built $OUT/wolfcrypt-m2354.elf"
    echo "flash with: nuvoton NuLink or OpenOCD, see README.md"
else
    echo "=== TrustZone compile check ==="

    # A TrustZone project must put its own partition_M2354.h ahead of the
    # BSP's, which errors out if it is the one that gets picked up. wolfSSL
    # does not ship a copy of that Nuvoton file, so take the BSP's and drop
    # the guard; a real project edits it for its own memory split.
    #
    # FMC_INIT_NSBA is also forced to 0. The BSP ships it as 1, and with that
    # set the FMC_NSBA_Setup() in system_M2354.c does not merely check the
    # non-secure boundary: if SCU->FNSADDR disagrees with FMC_SECURE_ROM_SIZE
    # it erases the config page, programs the new value and issues a chip
    # reset, from SystemInit(), before main(). A header derived here is for a
    # compile check and carries the BSP's default 0x80000, which is very
    # unlikely to be what a given part is actually set to, so leaving it on
    # would mean any image linked from it moved the part's secure boundary at
    # every boot. Setting the boundary is a provisioning step, not something a
    # build-test header should do.
    mkdir -p "$OUT/tz"
    sed -e '/# error "Link to default partition_M2354.h in secure mode/d' \
        -e 's/^#define FMC_INIT_NSBA .*/#define FMC_INIT_NSBA          0/' \
        "$STD/inc/partition_M2354.h" > "$OUT/tz/partition_M2354.h"

    grep -q '^#define FMC_INIT_NSBA          0' "$OUT/tz/partition_M2354.h" || {
        echo "failed to disable FMC_INIT_NSBA in the derived partition header" >&2
        exit 1
    }

    echo "--- non-secure: the port as wolfCrypt sees it"
    # shellcheck disable=SC2086
    compile "$OUT/obj-ns" "$CFLAGS -DWOLFSSL_NUVOTON_NSC" $PORT_SRC >/dev/null

    # The local partition header has to come first on the include path, ahead
    # of the BSP's, or the BSP's is the one that gets picked up.
    TZFLAGS="-I$OUT/tz -mcmse"
    TZFLAGS="$TZFLAGS -DWOLFSSL_NUVOTON_SECURE -DWOLFSSL_NUVOTON_NSC_IMPL"

    echo "--- secure: the NSC veneers"
    # shellcheck disable=SC2086
    $CC $TZFLAGS $CFLAGS \
        -c "$HERE/secure/nuvoton_nsc.c" -o "$OUT/obj-ns/nuvoton_nsc.o"

    echo "--- secure: the hardware layer the veneers call"
    # shellcheck disable=SC2086
    $CC $TZFLAGS $CFLAGS \
        -c "$ROOT/wolfcrypt/src/port/nuvoton/nuvoton_hw.c" \
        -o "$OUT/obj-ns/nuvoton_hw_s.o"

    echo
    echo "both halves compile"
fi
