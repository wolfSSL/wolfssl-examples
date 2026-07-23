#!/bin/bash
#
# Build and run every example that can run on this machine.
#
# Drives the same manifest CI does (.github/examples-manifest.yml), so what you
# get locally is what CI gets -- there is no second list to keep in sync.
#
#   ./scripts/run-all-examples.sh                 # every host example
#   ./scripts/run-all-examples.sh pkcs7           # just one, by manifest id
#   ./scripts/run-all-examples.sh --list          # what would run, and what won't
#
# Expects a wolfSSL installed at /usr/local (what every README tells you to do):
#
#   git clone --depth 1 https://github.com/wolfSSL/wolfssl && cd wolfssl
#   ./autogen.sh && ./configure --enable-all && make && sudo make install && sudo ldconfig
#
# Note the manifest defines three wolfSSL profiles. A plain --enable-all covers
# most examples; pk/rsa (fastmath) and signature/rsa_vfy_only (cryptonly) need
# their own builds -- see `profiles:` in the manifest.

set -uo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HARNESS="$REPO/.github/scripts/run_example.py"

if ! python3 -c 'import yaml' 2>/dev/null; then
    echo "PyYAML is required: pip install pyyaml" >&2
    exit 1
fi

if [ "${1:-}" = "--list" ]; then
    exec python3 - "$REPO" <<'PY'
import sys
sys.path.insert(0, f"{sys.argv[1]}/.github/scripts")
import manifest as m
ex = m.load()["examples"]
for mode, title in (("run", "RUNS"), ("build-only", "BUILDS ONLY"), ("skip", "NOT BUILT")):
    sel = [e for e in ex if e.get("mode", "run") == mode]
    print(f"\n{title} ({len(sel)})")
    for e in sorted(sel, key=lambda x: x["id"]):
        why = e.get("reason", "")
        print(f"  {e['id']:34s} {e['path']:44s} {why[:60]}")
PY
fi

if [ -n "${1:-}" ]; then
    exec python3 "$HARNESS" --only "$1"
fi

exec python3 "$HARNESS" --tier host
