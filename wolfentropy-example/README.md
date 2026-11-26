# wolfEntropy basic example

wolfEntropy is wolfSSL's software jitter entropy source, designed to SP 800-90B.
Once it is built in, `wc_InitRng()` seeds the DRBG from it automatically, so
most applications need no code changes at all. This example also shows the
direct API.

## 1. Build wolfSSL with wolfEntropy

```sh
git clone https://github.com/wolfSSL/wolfssl.git
cd wolfssl
./autogen.sh
./configure --enable-wolfEntropy --prefix=$(pwd)/wolf-install
make
make install
```

`--enable-wolfentropy` and `--enable-entropy-memuse` are aliases for the same
option. All three define `HAVE_ENTROPY_MEMUSE` (and turn on SHA-3, which
wolfEntropy uses for conditioning).

Optional values, comma separated, for example `--enable-wolfEntropy=nofallback`:

| Value | Effect |
|---|---|
| `nofallback` | If wolfEntropy fails, RNG seeding fails instead of falling back to the OS source (`/dev/urandom`). |
| `thread` | Use a counter thread for timing on targets with no high resolution hardware counter. |
| `amd-aperf` | Linux x86_64 only: use the AMD APERF counter. Cannot be combined with `thread`. |

For `user_settings.h` builds, define `HAVE_ENTROPY_MEMUSE` and `WOLFSSL_SHA3`,
and compile `wolfcrypt/src/wolfentropy.c` and `wolfcrypt/src/entropy-timers.c`.
To supply your own timer, define `CUSTOM_ENTROPY_TIMEHIRES` to the name of a
function returning `word64`.

## 2. Build and run the example

Set `WOLFSSL_INSTALL_DIR` in the Makefile to the `wolf-install` path above, or
pass it on the command line:

```sh
make WOLFSSL_INSTALL_DIR=/path/to/wolfssl/wolf-install
./wolfentropy_example
```

On Linux, also `export LD_LIBRARY_PATH=/path/to/wolfssl/wolf-install/lib`.

Sample output:

```
Entropy source:             wolfEntropy v7.0.0m
On demand health test:      passed
wc_Entropy_Get:             1c022efe38227d96abe0e2f00dab3ef6e2277f3d5e8d1cbee1d25c17afc4858f
wc_RNG_GenerateBlock:       c40de9576496a5f87098197698f6f0115f62f6ca2fdacdc266bac707363c6309
```

## API used

| Call | Purpose |
|---|---|
| `wolfCrypt_Init()` | Must come first. Sets up wolfEntropy and runs its startup health test. `wolfSSL_Init()` does this too. |
| `wc_Entropy_GetVersion()` | Version string, for logs. |
| `wc_Entropy_OnDemandTest()` | Re-runs the health tests on request. |
| `wc_Entropy_Get(bits, buf, len)` | Conditioned entropy straight from the source. `bits` is 1 to `MAX_ENTROPY_BITS` (256). |
| `wc_Entropy_GetRawEntropy(buf, cnt)` | Raw noise samples, for SP 800-90B entropy assessment only. Not for keys. |
| `wc_InitRng()` / `wc_RNG_GenerateBlock()` | The normal way to get random bytes; seeded by wolfEntropy. |

API reference: https://www.wolfssl.com/documentation/manuals/wolfssl/group__wolfEntropy.html
