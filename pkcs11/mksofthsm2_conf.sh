#!/bin/sh

echo "
# SoftHSM v2 configuration file

directories.tokendir = $PWD/softhsm2/
objectstore.backend = file

# ERROR, WARNING, INFO, DEBUG
log.level = ERROR

# If CKF_REMOVABLE_DEVICE flag should be set
slots.removable = false
" > softhsm2.conf

rm -rf ./softhsm2
mkdir softhsm2

