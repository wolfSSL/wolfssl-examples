#!/bin/sh

if [ $# -gt 0 ]
then
  SOFTHSM2_SLOTID=$1
  shift
fi

# the README builds SoftHSM from source into /usr/local, but a distro package
# lands in /usr/lib; SOFTHSM2_LIB overrides both
if [ -z "$SOFTHSM2_LIB" ]
then
  for lib in /usr/local/lib/softhsm/libsofthsm2.so \
             /usr/lib/softhsm/libsofthsm2.so \
             /usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so \
             /opt/homebrew/lib/softhsm/libsofthsm2.so
  do
    if [ -f "$lib" ]
    then
      SOFTHSM2_LIB=$lib
      break
    fi
  done
fi

if [ ! -f "$SOFTHSM2_LIB" ]
then
  echo "libsofthsm2.so not found, set SOFTHSM2_LIB to its path" >&2
  exit 1
fi

if [ -z "$SOFTHSM2_SLOTID" ]
then
  echo "no slot id, pass it as \$1 or set SOFTHSM2_SLOTID" >&2
  exit 1
fi

rc=0

run_example()
{
  name=$1
  shift
  echo
  echo "# $name"
  if ! "$@" "$SOFTHSM2_LIB" "$SOFTHSM2_SLOTID" SoftToken cryptoki
  then
    echo "# FAILED: $name"
    rc=1
  fi
}

echo "# Using slot ID: $SOFTHSM2_SLOTID"
echo "# Using library: $SOFTHSM2_LIB"

if [ $# -gt 0 ]
then
  for example in "$@"
  do
    run_example "$example" "./$example"
  done
else
  run_example "RSA example" ./pkcs11_rsa
  run_example "ECC example" ./pkcs11_ecc
  run_example "Generate ECC example" ./pkcs11_genecc
  run_example "AES-GCM example" ./pkcs11_aesgcm
  run_example "AES-CBC example" ./pkcs11_aescbc
  run_example "HMAC example" ./pkcs11_hmac
  run_example "Random Number Generation example" ./pkcs11_rand
  run_example "PKCS#11 test" ./pkcs11_test
fi

echo
if [ $rc -eq 0 ]
then
  echo "# All PKCS#11 examples passed"
else
  echo "# One or more PKCS#11 examples FAILED"
fi

exit $rc
