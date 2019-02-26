#!/bin/sh

#NOTE: Set if using an alternate install location to /usr/local/lib for libwolfssl
#Example:
#CUSTOM_INCLUDE_DIR="/Users/<you>/wolf-install-dir-for-testing/include"
#CUSTOM_LIB_DIR="/Users/<you>/wolf-install-dir-for-testing/lib"
CUSTOM_INCLUDE_DIR=""
CUSTOM_LIB_DIR=""


gcc rsa-private-encrypt-app.c -o rsa-priv-enc \
-I${CUSTOM_INCLUDE_DIR} \
-L${CUSTOM_LIB_DIR} -lwolfssl

gcc rsa-public-decrypt-app.c -o rsa-pub-dec \
-I${CUSTOM_INCLUDE_DIR} \
-L${CUSTOM_LIB_DIR} -lwolfssl

