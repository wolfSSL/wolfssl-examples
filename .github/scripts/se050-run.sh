#!/bin/sh
# Build the SE050 stack -- SE050Sim (Rust TCP applet sim on :8050), the NXP
# plug-and-trust SDK v04.07.01 with wolfSSL's HostCrypto patch, and wolfSSL
# --with-se050 -- then build this repo's SE050 wolfcrypt_test demo and run it
# against the sim. Recipe mirrors SE050Sim/Dockerfile.wolfcrypt (the sim's own
# CI), grafting this repo's ex_sss demo in place of the sim's standalone main.
set -eu

ROOT="$PWD"
WORK=/tmp/se050
mkdir -p "$WORK"
cd "$WORK"

echo "::group::SE050Sim (Rust TCP server)"
"$ROOT/.github/scripts/git-clone-retry.sh" -q --depth 1 https://github.com/wolfSSL/simulators sims
# se050-sim depends on the se050 crate at ../nxp-se050/se050 (its own repo)
"$ROOT/.github/scripts/git-clone-retry.sh" -q --branch sim-compat --depth 1 \
  https://github.com/LinuxJedi/nxp-se050.git sims/SE050Sim/nxp-se050
( cd sims/SE050Sim/se050-sim && cargo build --release --bin tcp_server )
SIM="$WORK/sims/SE050Sim/wolfcrypt-test"
echo "::endgroup::"

echo "::group::plug-and-trust SDK + wolfSSL HostCrypto patch + sim overlays"
"$ROOT/.github/scripts/git-clone-retry.sh" -q --depth 1 --branch v04.07.01 https://github.com/NXP/plug-and-trust simw-top
"$ROOT/.github/scripts/retry.sh" curl -fsSL -o "$WORK/osp-se050.patch" \
  https://raw.githubusercontent.com/wolfSSL/osp/master/nxp-se05x-middleware/simw-top-v040701.patch
( cd simw-top && patch -p1 -l --forward --fuzz=3 < "$WORK/osp-se050.patch" )
test -f simw-top/sss/src/wolfssl/fsl_sss_wolfssl_apis.c
cp "$SIM/i2c_a7.c"      simw-top/hostlib/hostLib/platform/linux/i2c_a7.c
cp "$SIM/se05x_reset.c" simw-top/hostlib/hostLib/platform/rsp/se05x_reset.c
python3 "$SIM/patch_ftr.py" simw-top/fsl_sss_ftr.h
cp "$SIM/CMakeLists.txt" simw-top/CMakeLists.txt
echo "::endgroup::"

echo "::group::wolfSSL pass A (no se050, so the SDK can link -lwolfssl)"
"$ROOT/.github/scripts/git-clone-retry.sh" -q --depth 1 https://github.com/wolfSSL/wolfssl wolfssl
( cd wolfssl && ./autogen.sh >/dev/null 2>&1 && \
  ./configure --enable-keygen --enable-cmac \
    CFLAGS="-DWOLFSSL_SE050_NO_TRNG -DSIZEOF_LONG_LONG=8" >/dev/null && \
  make -j"$(nproc)" >/dev/null && sudo make install >/dev/null && sudo ldconfig )
echo "::endgroup::"

echo "::group::six SDK static libs (WOLFSSL HostCrypto)"
mkdir -p simw-top/build
( cd simw-top/build && \
  cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-fPIC" \
    -DPTMW_Applet=SE05X_C -DPTMW_SE05X_Auth=None \
    -DPTMW_SMCOM=T1oI2C -DPTMW_HostCrypto=WOLFSSL -DPTMW_Host=LinuxLike >/dev/null && \
  cmake --build . -j"$(nproc)" >/dev/null && \
  cp ../fsl_sss_ftr.h ./ && \
  ln -sf "$WORK/simw-top/build/sss/ex/src/libex_common.a" \
         "$WORK/simw-top/build/sss/libex_common.a" )
echo "::endgroup::"

echo "::group::wolfSSL pass B (--with-se050)"
( cd wolfssl && make clean >/dev/null && \
  ./configure --with-se050="$WORK/simw-top" \
    --enable-keygen --enable-cmac --enable-cryptocb --enable-ecc \
    --enable-ed25519 --enable-curve25519 --enable-sha224 --enable-sha384 --enable-sha512 \
    --disable-examples --enable-crypttests \
    CFLAGS="-DWOLFSSL_SE050_INIT -DWOLFSSL_SE050_NO_TRNG -DSIZEOF_LONG_LONG=8 \
            -DECC_USER_CURVES -DHAVE_ECC224 -DHAVE_ECC256 -DHAVE_ECC384" \
    LDFLAGS="-L$WORK/simw-top/build" >/dev/null && \
  make -j"$(nproc)" >/dev/null && sudo make install >/dev/null && sudo ldconfig )
echo "::endgroup::"

echo "::group::build this repo's wolfcrypt_test demo"
D="$ROOT/SE050/wolfssl/wolfcrypt_test"
cp wolfssl/wolfcrypt/test/test.c "$D/test.c"
cp wolfssl/wolfcrypt/test/test.h "$D/test.h"
B="$WORK/simw-top/build"
gcc -o "$WORK/wolfcrypt_test" "$D/wolfcrypt_test.c" "$D/test.c" \
  -DSIMW_DEMO_ENABLE__DEMO_WOLFCRYPTTEST -DNO_MAIN_DRIVER -DBENCH_EMBEDDED \
  -DUSE_CERT_BUFFERS_2048 -DUSE_CERT_BUFFERS_256 -DHAVE_CONFIG_H \
  -I"$D" -I"$WORK/wolfssl" -I"$WORK/wolfssl/wolfcrypt/test" -I/usr/local/include \
  -I"$WORK/simw-top" -I"$WORK/simw-top/sss/inc" -I"$WORK/simw-top/sss/ex/inc" \
  -I"$WORK/simw-top/sss/port/default" -I"$WORK/simw-top/hostlib/hostLib/inc" \
  -I"$WORK/simw-top/hostlib/hostLib/libCommon/log" \
  -I"$WORK/simw-top/hostlib/hostLib/libCommon/infra" \
  -I"$WORK/simw-top/hostlib/hostLib/se05x_03_xx_xx" \
  -I"$WORK/simw-top/hostlib/hostLib/platform/inc" -I"$B" \
  -L/usr/local/lib -L"$B/sss" -L"$B/hostlib/hostLib/se05x" -L"$B/hostlib/hostLib" \
  -L"$B/hostlib/hostLib/libCommon/log" -L"$B/hostlib/hostLib/libCommon" \
  -lwolfssl -lSSS_APIs -lex_common -lse05x -la7x_utils -lmwlog -lsmCom -lpthread -lm
echo "::endgroup::"

echo "::group::run wolfcrypt_test against SE050Sim"
"$WORK/sims/SE050Sim/se050-sim/target/release/tcp_server" >/tmp/se050sim.log 2>&1 &
simpid=$!
sleep 2
export SE050_SIM_HOST=127.0.0.1 SE050_SIM_PORT=8050 LD_LIBRARY_PATH=/usr/local/lib
out=$("$WORK/wolfcrypt_test" 2>&1) || true
kill "$simpid" 2>/dev/null || true
printf '%s\n' "$out" | tail -40
printf '%s' "$out" | grep -qiE "Ran wolfCrypt test|Test complete|error test passed" \
  || { echo "FAIL: se050 wolfcrypt_test did not report success"; exit 1; }
echo "PASS: se050 wolfcrypt_test ran against SE050Sim"
echo "::endgroup::"
