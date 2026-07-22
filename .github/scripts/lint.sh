#!/bin/sh
# Run shellcheck (errors) over the repo's shell scripts and actionlint over the
# workflows. Examples are copy-paste source for users, so a broken helper script
# propagates. Blocking on shellcheck ERRORS only; the pre-existing warning
# backlog and these three long-standing legacy scripts are left for a dedicated
# cleanup rather than a drive-by here.
set -eu

# Long-standing errors unrelated to this CI work; excluded so new scripts are
# still gated. See task: clean the lint backlog and drop these.
LEGACY="./meta-wolfssl-linux-fips/configure.sh ./tirtos_ccs_examples/nameReplace.sh ./psa/build_with_mbedtls_psa.sh"

rc=0
for s in $(find . -name '*.sh' -not -path './.git/*' | sort); do
    skip=0
    for l in $LEGACY; do [ "$s" = "$l" ] && skip=1; done
    [ "$skip" -eq 1 ] && continue
    if ! shellcheck -S error "$s"; then rc=1; fi
done
[ "$rc" -eq 0 ] && echo "shellcheck: no errors outside the documented legacy scripts"

# actionlint runs shellcheck on inline run: blocks too; that backlog is warnings,
# so report it without failing the build.
if command -v actionlint >/dev/null 2>&1; then
    actionlint || echo "actionlint: reported issues above (informational)"
fi

# cppcheck over the example C. Informational: without each example's exact build
# defines cppcheck misreads wolfSSL's config macros (MYNEWT_VAL, LIBWOLFSSL_*),
# so the parse-level categories are suppressed and the rest never fails the build.
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --quiet --enable=warning \
        --suppress=missingInclude --suppress=missingIncludeSystem \
        --suppress=syntaxError --suppress=unknownMacro \
        --suppress=preprocessorErrorDirective \
        $(find . -name '*.c' -not -path './.git/*' -not -path '*/pico-sdk/*' -not -path '*/mcux-sdk/*') \
        || echo "cppcheck: reported issues above (informational)"
fi

# codespell over the repo's own text. Informational: vendored SDKs and binary
# assets are skipped, and common-word false positives should not fail the build.
if command -v codespell >/dev/null 2>&1; then
    codespell --skip='./.git,*/pico-sdk/*,*/mcux-sdk/*,*.der,*.pem,*.raw,*.enc' \
        . || echo "codespell: reported issues above (informational)"
fi

# make -n parses every Makefile without building. Informational: a few dirs
# (RT1060, riot, SGX, uefi) are driven by external SDKs whose fragments a bare
# make cannot resolve, so a parse miss there is expected, not a defect.
makefail=""
for mk in $(find . -name Makefile -not -path './.git/*' -not -path '*/wolfssl/*' | sort); do
    d=$(dirname "$mk")
    make -n -C "$d" >/dev/null 2>&1 || makefail="$makefail $d"
done
if [ -n "$makefail" ]; then
    echo "make -n could not parse (informational, usually external-SDK dirs):$makefail"
else
    echo "make -n: every Makefile parsed"
fi

exit $rc
