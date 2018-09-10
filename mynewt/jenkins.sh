#!/bin/bash -ex

BASEDIR=`dirname $0`
BASEDIR=`cd $BASEDIR && pwd -P`

# kill previous process
set +e
killall -9 wolfsslclienttlsmn.elf
set -e

pushd ${BASEDIR} > /dev/null

/bin/rm -rf tmp
/bin/mkdir tmp
pushd tmp > /dev/null

# create mynewt project
newt new myproj
NEWTPROJ=`pwd`/myproj
pushd ${NEWTPROJ} > /dev/null
newt upgrade
popd > /dev/null

# deploy wolfssl source files to mynewt project
git clone https://github.com/wolfSSL/wolfssl.git
WOLFSSL=`pwd`/wolfssl
${WOLFSSL}/IDE/mynewt/setup.sh ${NEWTPROJ}

# deploy wolfssl example source files to mynewt project
${BASEDIR}/setup.sh ${NEWTPROJ}

# build sample program
pushd ${NEWTPROJ} > /dev/null
newt target create wolfsslclienttlsmn_sim
newt target set wolfsslclienttlsmn_sim app=apps/wolfsslclienttlsmn
newt target set wolfsslclienttlsmn_sim bsp=@apache-mynewt-core/hw/bsp/native
newt target set wolfsslclienttlsmn_sim build_profile=debug
newt build wolfsslclienttlsmn_sim

/bin/rm -f wolfsslclienttlsmn.log
(./bin/targets/wolfsslclienttlsmn_sim/app/apps/wolfsslclienttlsmn/wolfsslclienttlsmn.elf &) > wolfsslclienttlsmn.log
sleep 1
TTY_NAME=`cat wolfsslclienttlsmn.log | cut -d ' ' -f 3`

expect ${BASEDIR}/test_client-tls.expect $TTY_NAME

killall -9 wolfsslclienttlsmn.elf

popd > /dev/null

popd > /dev/null # tmp

# cleanup tmp directory on jenkins
if [ ! -z "$JENKINS_URL" ]; then
    /bin/rm -rf tmp
fi  

popd > /dev/null # ${BASEDIR}
