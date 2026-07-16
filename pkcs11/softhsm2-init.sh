#!/bin/sh

set -e

cd "$(dirname "$0")"

./mksofthsm2_conf.sh
SOFTHSM2_CONF="$PWD/softhsm2.conf"
export SOFTHSM2_CONF

out=$(softhsm2-util --init-token --slot 0 --label SoftToken \
        --so-pin cryptoki --pin cryptoki)
echo "$out"

# SoftHSM reassigns the token to a random slot, so the id cannot be hardcoded
slotid=$(echo "$out" | sed -n 's/.*reassigned to slot \([0-9][0-9]*\).*/\1/p')
if [ -z "$slotid" ]
then
  echo "could not read the reassigned slot id from softhsm2-util" >&2
  exit 1
fi

exec ./softhsm2.sh "$slotid" "$@"
