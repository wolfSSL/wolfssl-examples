#!/usr/bin/env bash
# Retry a re-runnable command (downloads, docker build) up to RETRY_TRIES times.
set -u
tries="${RETRY_TRIES:-5}"
delay="${RETRY_DELAY:-10}"
i=1
while true; do
    "$@" && exit 0
    rc=$?
    if [ "$i" -ge "$tries" ]; then
        echo "retry: '$*' failed after $tries attempts (rc=$rc)" >&2
        exit "$rc"
    fi
    echo "retry: '$*' failed (rc=$rc); attempt $i/$tries, retrying in ${delay}s" >&2
    i=$((i + 1))
    sleep "$delay"
done
