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
for cmd in newt expect screen git openssl ss xxd python3 fuser; do
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

# Fix compiler -Werror for sim targets on 64-bit Linux.
# Upstream PR: https://github.com/apache/mynewt-core/pull/3713
sed -i -E 's/-Werror([[:space:]",]|$)/-Wno-error\1/g' repos/apache-mynewt-core/compiler/sim/compiler.yml
if grep -qE -- '-Werror([[:space:]",]|$)' repos/apache-mynewt-core/compiler/sim/compiler.yml; then
    echo "Error: -Werror patch did not apply to compiler.yml (upstream may have changed)."
    exit 1
fi

# Fix LINK_TABLE start pointer alignment in shell module tables.
# Upstream PR: https://github.com/apache/mynewt-core/pull/3711
python3 - repos/apache-mynewt-core/sys/shell/src/shell.c <<'PYEOF'
import re, sys

path = sys.argv[1]
with open(path) as f:
    src = f.read()

old = """static size_t
shell_mod_std_count(shell_mod_t mod)
{
    const struct shell_mod_std *std_mod = (const struct shell_mod_std *)mod;
    return std_mod->commands_end - std_mod->commands;
}

static shell_cmd_t
shell_mod_std_get(shell_mod_t mod, size_t ix)
{
    const struct shell_mod_std *std_mod = (const struct shell_mod_std *)mod;
    size_t limit = std_mod->commands_end - std_mod->commands;
    return ix < limit ? &std_mod->commands[ix] : NULL;
}"""

new = """static const struct shell_cmd *
shell_mod_std_commands(const struct shell_mod_std *std_mod)
{
    uintptr_t addr = (uintptr_t)std_mod->commands;
    uintptr_t aligned = (addr + sizeof(struct shell_cmd) - 1) &
                         ~(uintptr_t)(sizeof(struct shell_cmd) - 1);

    return (const struct shell_cmd *)aligned;
}

static size_t
shell_mod_std_count(shell_mod_t mod)
{
    const struct shell_mod_std *std_mod = (const struct shell_mod_std *)mod;
    return std_mod->commands_end - shell_mod_std_commands(std_mod);
}

static shell_cmd_t
shell_mod_std_get(shell_mod_t mod, size_t ix)
{
    const struct shell_mod_std *std_mod = (const struct shell_mod_std *)mod;
    const struct shell_cmd *commands = shell_mod_std_commands(std_mod);
    size_t limit = std_mod->commands_end - commands;
    return ix < limit ? &commands[ix] : NULL;
}"""

if old not in src:
    print("Error: shell_mod_std_count/get patch target not found in shell.c "
          "(upstream may have changed).", file=sys.stderr)
    sys.exit(1)

with open(path, "w") as f:
    f.write(src.replace(old, new, 1))
PYEOF

# Fix uart_pty() loop_slave FD leak.
# Upstream PR: https://github.com/apache/mynewt-core/pull/3712
sed -i 's|snprintf(msg, sizeof(msg), "uart%d at %s\\n", port, pty_name);|close(loop_slave);\n    snprintf(msg, sizeof(msg), "uart%d at %s\\n", port, pty_name);|' \
    repos/apache-mynewt-core/hw/mcu/native/src/hal_uart.c
if ! grep -q 'close(loop_slave);' repos/apache-mynewt-core/hw/mcu/native/src/hal_uart.c; then
    echo "Error: loop_slave close patch did not apply to hal_uart.c (upstream may have changed)."
    exit 1
fi

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
