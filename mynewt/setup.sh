#!/bin/bash -e

# this scrypt deploy wolfssl and wolfcrypto source code to mynewt project
# run as bash "mynewt project root directory path"

SCRIPTDIR=`dirname $0`
SCRIPTDIR=`cd $SCRIPTDIR && pwd -P`
WOLFSSL_MYNEWTDIR=${SCRIPTDIR}
WOLFSSL_MYNEWTDIR=`cd $WOLFSSL_MYNEWTDIR && pwd -P`
BASEDIR=${SCRIPTDIR}/../..
BASEDIR=`cd ${BASEDIR} && pwd -P`

if [ $# -ne 1 ]; then
    echo "Usage: $0 'mynewt project root directory path'" 1>&2
    exit 1
fi
MYNEWT_PROJECT=$1

if [ ! -d $MYNEWT_PROJECT ] || [ ! -f $MYNEWT_PROJECT/project.yml ]; then
    echo "target directory is not mynewt project.: $MYNEWT_PROJECT"
    exit 1
fi

pushd $MYNEWT_PROJECT > /dev/null

echo "create apps/wolfsslclienttlsmn pkg"
/bin/rm -rf apps/wolfsslclienttlsmn
newt pkg new -t app apps/wolfsslclienttlsmn
/bin/rm -rf apps/wolfsslclienttlsmn/include 
/bin/rm -rf apps/wolfsslclienttlsmn/src
/bin/mkdir -p apps/wolfsslclienttlsmn/src

popd > /dev/null # $MYNEWT_PROJECT

# deploy source files and pkg
pushd $BASEDIR > /dev/null

# deploy wolfssl client-tls to apps/wolfsslclienttlsmn
echo "deploy wolfssl client-tls to apps/wolfsslclienttlsmn"
/bin/cp $WOLFSSL_MYNEWTDIR/apps.wolfsslclienttlsmn.pkg.yml $MYNEWT_PROJECT/apps/wolfsslclienttlsmn/pkg.yml
/bin/cp $WOLFSSL_MYNEWTDIR/apps.wolfsslclienttlsmn.syscfg.yml $MYNEWT_PROJECT/apps/wolfsslclienttlsmn/syscfg.yml
/bin/mkdir -p $MYNEWT_PROJECT/apps/wolfsslclienttlsmn/src

/bin/cp $WOLFSSL_MYNEWTDIR/client-tls-mn.c $MYNEWT_PROJECT/apps/wolfsslclienttlsmn/src/main.c

popd > /dev/null # $BASEDIR
