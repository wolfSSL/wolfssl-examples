#!/bin/sh
# git clone with retry; removes the target dir (last arg) between tries so a partial clone cannot block the retry.
set -u
target=""
for target in "$@"; do :; done
case "$target" in http*|git@*|ssh://*|"") target="" ;; esac
tries="${RETRY_TRIES:-5}"
delay="${RETRY_DELAY:-10}"
i=1
while true; do
    [ -n "$target" ] && rm -rf "$target"
    git clone "$@" && exit 0
    rc=$?
    if [ "$i" -ge "$tries" ]; then
        echo "git clone failed after $tries attempts (rc=$rc)" >&2
        exit "$rc"
    fi
    echo "git clone attempt $i/$tries failed (rc=$rc); retrying in ${delay}s" >&2
    i=$((i + 1))
    sleep "$delay"
done
