# AUTOSAR port on a `user_settings.h` build

An ECU build has no `configure` and no CMake. It compiles the wolfSSL sources it
needs straight into the project, configured by a `user_settings.h` on the
include path, with `-DWOLFSSL_USER_SETTINGS` on every translation unit. This
directory is that build, reduced to the minimum the AUTOSAR port needs.

```sh
$ make WOLFSSL_ROOT=/path/to/wolfssl
$ make WOLFSSL_ROOT=/path/to/wolfssl check
```

`WOLFSSL_ROOT` defaults to `../../../wolfssl`, so if the wolfSSL and
wolfssl-examples checkouts sit side by side, plain `make` works.

Cross-compiling needs a target compiler, target flags on both the compile and
the link, and two things this host build gets for free: `ecu-seed.c` reads
`/dev/urandom`, so replace it with your entropy source (section below), and a
bare-metal link needs its own specs or linker script. The shape is:

```sh
$ make WOLFSSL_ROOT=/path/to/wolfssl CC=arm-none-eabi-gcc \
       CFLAGS="-Os -mcpu=cortex-m4 -mthumb -ffunction-sections" \
       LDFLAGS="--specs=nosys.specs -Wl,--gc-sections"
```

`make size` reports what the result costs, which is the number you actually want
when sizing flash. The figure is toolchain-specific, so measure it with yours.

| File | |
| --- | --- |
| `user_settings.h` | the configuration; the interesting part |
| `ecu-app.c` | a minimal SW-C that exercises everything the configuration claims |
| `ecu-seed.c` | the entropy source, the one piece you must replace |
| `Makefile` | the source list and flags |

## The source list

Six wolfCrypt files plus the port's three:

```
aes.c  cmac.c  memory.c  random.c  sha256.c  wc_port.c
port/autosar/{csm,cryif,crypto}.c
```

That was established by removing each candidate in turn and checking the link,
not by guessing: `hash.c`, `hmac.c` and `wc_encrypt.c` were in an earlier draft
of the list and are not needed. `logging.c` is in the Makefile because a
`-DDEBUG_WOLFSSL` build references it; without that macro `WOLFSSL_MSG()`
compiles to nothing and you can drop it.

`misc.c` is deliberately absent — it is `#include`d into `crypto.c` unless
`NO_INLINE` is defined, so compiling it separately gives duplicate symbols.

Re-check the list if you change the feature set. Dropping
`WOLFSSL_AUTOSAR_CMAC`, for instance, makes `cmac.c` unnecessary.

## Five things that bite

Every one of these was hit while getting this directory to build and run.

**1. `NO_FILESYSTEM` removes the entropy source.** `random.c` guards its
`/dev/urandom` reader with `#ifndef NO_FILESYSTEM`, and the `#else` returns
`NOT_COMPILED_IN`. So a filesystem-free build has no seed unless you name one
with `CUSTOM_RAND_GENERATE_SEED`. This is **not a build error** — it links
cleanly, then `wc_InitRng()` returns `RNG_FAILURE_E` and every
`Csm_RandomGenerate()` returns `E_NOT_OK`. `ecu-seed.c` is the hook.

**2. The Hash_DRBG is already on; what you can lose is its seed.**
`random.h` defines `HAVE_HASHDRBG` for you unless you define `WC_NO_HASHDRBG`
or `CUSTOM_RAND_GENERATE_BLOCK`, so there is nothing to turn on here -- an
earlier version of this README claimed otherwise. The real trap is the seed:
`NO_FILESYSTEM` compiles out `random.c`'s `/dev/urandom` reader, the build
still links, and every `Csm_RandomGenerate()` then fails with `RNG_FAILURE_E`.
That is what `CUSTOM_RAND_GENERATE_SEED` and `ecu-seed.c` are for.

**3. `WOLFSSL_AES_128/192/256` decide which keys the driver will accept.** The
Crypto driver takes the AES variant from the length of the key in the keystore,
so a key length you did not enable is stored happily by
`Csm_KeyElementSet()` and then rejected by the job that tries to use it. Enable
the lengths the ECU will be provisioned with.

**4. `WOLFSSL_CMAC` needs `WOLFSSL_AES_DIRECT` with it.** The `WC_CMAC_AES` case
in `cmac.c` is compiled out otherwise, and `wc_InitCmac()` fails at runtime. The
`--enable-autosar-cmac` configure option handles this for you; a hand-written
`user_settings.h` does not.

**5. `-DWOLFSSL_USER_SETTINGS` must reach the library sources and the
application.** If it reaches only one, the two compile against different
configurations, structs change size, and it links and then misbehaves. The
Makefile keeps those flags in `ALL_CFLAGS` rather than `CFLAGS` precisely so
that `make CFLAGS=...` on the command line cannot silently drop them — an
earlier version of this Makefile could, and the resulting failure was hard to
read.

## What to change for a real target

* **`ecu-seed.c`.** It reads `/dev/urandom` so this example runs on a
  workstation. On a vehicle the seed comes from an on-chip TRNG, an HSM or SHE.
  The DRBG is only as unpredictable as what you feed it, so a counter or a
  timestamp here makes every key and IV on the ECU predictable.
* **`WOLFSSL_AUTOSAR_DEVID`.** Left at the default (`INVALID_DEVID`, software).
  Define it to a registered crypto callback device ID and every job the Crypto
  driver runs goes to that HSM or accelerator instead — which is how this build
  stops holding key material in RAM. Needs `WOLF_CRYPTO_CB`, and the
  application must call `wolfCrypt_Init()` and `wc_CryptoCb_RegisterDevice()`
  before `Csm_Init()`.
* **`SINGLE_THREADED`.** Set for simplicity. An RTOS build sets its own macro
  instead (`FREERTOS`, `WOLFSSL_ZEPHYR`, `THREADX`, ...) so the driver's
  keystore and job-table mutexes become real locks. See `../csm-threads.c` for
  what they protect and why it matters once two SW-Cs share the CSM.
* **`MAX_KEYSTORE` and `MAX_JOBS`.** Set to 4 and 2 here to show they are
  tunable. `MAX_JOBS` must be at least the number of jobs that can be streaming
  between START and FINISH at once.
* **Key provisioning.** `ecu-app.c` compiles its keys in, which no production
  ECU should. Keys arrive through whatever provisioning path the vehicle
  defines and land in the keystore via `Csm_KeyElementSet()`.

## Checking the configuration

`ecu-app` exercises AES-128, AES-192 and AES-256 CBC round trips, the DRBG, and
an RFC 4493 AES-CMAC known-answer vector — so a mis-trimmed `user_settings.h`
fails here rather than in the field:

```
AUTOSAR CSM on a user_settings.h build
  wolfSSL 5.9.4, MAX_KEYSTORE=4, MAX_JOBS=2

  AES-128-CBC round trip             ok
  AES-192-CBC round trip             ok
  AES-256-CBC round trip             ok
  Csm_RandomGenerate                 ok
  AES-CMAC known answer              ok

ecu-app: PASS
```

Each check is compiled in only when the corresponding macro is set, so removing
a feature from `user_settings.h` removes its check rather than failing the run.
If something reports `FAILED`, build with
`CFLAGS="-Os -Wall -DDEBUG_WOLFSSL"` and call `wolfSSL_Debugging_ON()` to get
the driver's reason.
