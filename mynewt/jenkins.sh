#!/bin/bash -ex
BASEDIR=`dirname $0`
BASEDIR=`cd $BASEDIR && pwd -P`

# Resolve local WOLFSSL_REPO to absolute path.
WOLFSSL_REPO=${WOLFSSL_REPO:-https://github.com/wolfSSL/wolfssl.git}
WOLFSSL_BRANCH=${WOLFSSL_BRANCH:-master}
if [ -d "$WOLFSSL_REPO" ]; then
    WOLFSSL_REPO=$(cd "$WOLFSSL_REPO" && pwd -P)
fi

# check for required dependencies
for cmd in newt expect screen git openssl ss xxd fuser; do
    if ! command -v $cmd &> /dev/null; then
        echo "Error: Required command '$cmd' is not installed."
        echo "Please install it before running this script."
        exit 1
    fi
done

# kill previous process
set +e
killall -9 wolfsslclienttlsmn.elf
set -e

pushd ${BASEDIR} > /dev/null

/bin/rm -rf tmp/myproj
/bin/mkdir -p tmp
pushd tmp > /dev/null

# create mynewt project
newt new myproj
NEWTPROJ=`pwd`/myproj
pushd ${NEWTPROJ} > /dev/null

# This example needs apache-mynewt-core master ("vers: 0.0.0"), not the release
# "newt new" pins. Same file newt generates, with the version bumped.
cat > project.yml <<'EOF'
project.name: "my_project"

project.repositories:
    - apache-mynewt-core

repository.apache-mynewt-core:
    type: github
    vers: 0.0.0
    user: apache
    repo: mynewt-core
EOF

newt upgrade
popd > /dev/null

# deploy wolfssl source files to mynewt project
if [ -d "$WOLFSSL_REPO" ]; then
    echo "Using local wolfssl repository at $WOLFSSL_REPO"
    WOLFSSL="$WOLFSSL_REPO"
else
    if [ -d "wolfssl" ]; then
        echo "wolfssl already cloned, updating..."
        pushd wolfssl > /dev/null
        git fetch --depth 1 origin $WOLFSSL_BRANCH
        git checkout -B $WOLFSSL_BRANCH FETCH_HEAD
        popd > /dev/null
    else
        echo "Cloning wolfssl..."
        git clone --depth 1 -b $WOLFSSL_BRANCH $WOLFSSL_REPO
    fi
    WOLFSSL=`pwd`/wolfssl
fi

${WOLFSSL}/IDE/mynewt/setup.sh ${NEWTPROJ}

# deploy wolfssl example source files to mynewt project
${BASEDIR}/setup.sh ${NEWTPROJ}

# build sample program
pushd ${NEWTPROJ} > /dev/null
newt target create wolfsslclienttlsmn_sim
newt target set wolfsslclienttlsmn_sim app=apps/wolfsslclienttlsmn
newt target set wolfsslclienttlsmn_sim bsp=@apache-mynewt-core/hw/bsp/native
newt target set wolfsslclienttlsmn_sim build_profile=debug

# Test that WOLFSSL_MN_USE_CUSTOM_CA path halts compilation with #error as intended
newt target set wolfsslclienttlsmn_sim syscfg=WOLFSSL_MN_USE_CUSTOM_CA=1
if newt build wolfsslclienttlsmn_sim > /dev/null 2>&1; then
    echo "Error: WOLFSSL_MN_USE_CUSTOM_CA build succeeded but was expected to fail with #error."
    exit 1
fi
newt target set wolfsslclienttlsmn_sim syscfg=WOLFSSL_MN_USE_CUSTOM_CA=0

newt build wolfsslclienttlsmn_sim

/bin/rm -f wolfsslclienttlsmn.log
(./bin/targets/wolfsslclienttlsmn_sim/app/apps/wolfsslclienttlsmn/wolfsslclienttlsmn.elf &) > wolfsslclienttlsmn.log
sleep 1
TTY_NAME=`cat wolfsslclienttlsmn.log | cut -d ' ' -f 3`

# Free port 11111 of any stale listener left behind by an aborted previous run.
set +e
fuser -k 11111/tcp 2> /dev/null
set -e

# Start a local TLS server on port 11111 for the client to connect to
pushd ${BASEDIR}/../tls > /dev/null
make
./server-tls &
OSSL_PID=$!
popd > /dev/null
trap "kill $OSSL_PID 2> /dev/null || true" EXIT

# Wait for server to listen via socket state, avoiding probing TCP connections.
for i in $(seq 1 30); do
    if ! kill -0 $OSSL_PID 2>/dev/null; then
        echo "Error: server-tls (PID $OSSL_PID) died unexpectedly."
        exit 1
    fi
    if ss -ltn "sport = :11111" 2> /dev/null | grep -q ':11111'; then
        break
    fi
    if [ "$i" -eq 30 ]; then
        echo "Error: server-tls never started listening on port 11111."
        exit 1
    fi
    sleep 0.5
done

export TERM=vt100
expect ${BASEDIR}/test_client-tls.expect $TTY_NAME

kill $OSSL_PID 2> /dev/null || true
trap - EXIT

echo ""
echo "=========================================================="
echo "✅ SUCCESS: All TLS tests passed!"
echo "A transcript of the test session was saved to:"
echo "tmp/myproj/expect_output.log"
echo "=========================================================="
echo ""

killall -9 wolfsslclienttlsmn.elf

popd > /dev/null

popd > /dev/null # tmp

# cleanup tmp directory on jenkins
if [ ! -z "$JENKINS_URL" ]; then
    [ -f tmp/myproj/expect_output.log ] && mv tmp/myproj/expect_output.log expect_output.log
    /bin/rm -rf tmp
fi  

popd > /dev/null # ${BASEDIR}
