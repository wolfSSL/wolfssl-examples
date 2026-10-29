# wolfSSL AUTOSAR CSM Examples

Examples using the wolfSSL AUTOSAR Classic Platform port, which plugs wolfCrypt
in underneath the standard `Csm` → `CryIf` → `Crypto` driver chain (AUTOSAR
release 4.4). An application SW-C calls the standardized `Csm_*` API and
wolfCrypt does the work.

The port itself lives in `wolfcrypt/src/port/autosar/` in the wolfSSL
repository, along with its own
[README](https://github.com/wolfSSL/wolfssl/blob/master/wolfcrypt/src/port/autosar/README.md).

| Example | Shows |
| --- | --- |
| `csm-basic.c` | `Csm_Init`, `Csm_GetVersionInfo`, `Csm_RandomGenerate`, `Csm_KeyElementSet`, and AES-CBC via `Csm_Encrypt`/`Csm_Decrypt` — both single-call and streamed |
| `csm-key-redirection.c` | How the Crypto driver picks a keystore slot, and how compile-time key input redirection pins a job to a specific slot |
| `csm-jobs.c` | Two jobs streaming interleaved, and what happens when the `MAX_JOBS` table fills up |
| `csm-errors.c` | Every way the port returns `E_NOT_OK`, and the one failure that is not an `E_NOT_OK` |
| `csm-secoc.c` | Secure Onboard Communication over classic CAN: truncated CMAC plus a freshness value, and the attacks each half stops. Needs `--enable-autosar-cmac` |
| `csm-stream.c` | A payload larger than a CAN frame, encrypted a block at a time — and what one lost frame does to a CBC stream |
| `csm-threads.c` | Several SW-Cs using the CSM concurrently: what the driver's two mutexes protect, and what they do not |
| `csm-she-provision.c` | Where the keys come from: a SHE key update block, checked against the specification's vector, then handed to the CSM. Needs `--enable-she=standard` |
| `csm-cryptocb.c` | Backing the Crypto driver with hardware: which operations reach your callback, and how to accept, decline or fail them. Needs `--enable-cryptocb` |
| [`user_settings/`](user_settings/README.md) | The same port built without autotools or CMake, the way an ECU builds it |

## What the port supports

Worth knowing before you write against it, because the surface is narrow:

* **AES-CBC, DRBG random, and optionally AES-CMAC.** `Csm.h` defines the full
  set of AUTOSAR algorithm and service enums, but only `CRYPTO_ENCRYPT`,
  `CRYPTO_DECRYPT` and `CRYPTO_RANDOMGENERATE` have a driver behind them by
  default. `--enable-autosar-cmac` adds `CRYPTO_MACGENERATE` and
  `CRYPTO_MACVERIFY`. Everything else — hash, AEAD, signature, key derivation,
  certificate services — still sits behind `CSM_UNSUPPORTED_ALGS` with nothing
  implementing it.
* **Synchronous only.** `CRYPTO_PROCESSING_SYNC` is hardcoded; `CryIf` rejects
  async jobs and `CryIf_CancelJob` is a stub.
* **No padding.** `CRYPTO_ALGOFAM_PADDING_PKCS7` exists as an enum value only.
  Every buffer you hand to `Csm_Encrypt`/`Csm_Decrypt` must be a whole number
  of 16-byte blocks; pad in the application.
* **`resultLengthPtr` is not an output for cipher jobs.** The AUTOSAR spec
  treats it as in/out, but this port never writes back through it for
  encrypt/decrypt. For CBC the output length equals the input length, so track
  it yourself. `Csm_RandomGenerate` reads it as the number of bytes wanted and
  likewise does not update it.
* **Compile-time sizing.** `MAX_KEYSTORE` (default 15) key slots and
  `MAX_JOBS` (default 10) concurrent streaming jobs, both overridable at build
  time.
* **The stored key's length picks the AES variant.** A cipher job asks for
  whatever the keystore holds, so a 16, 24 or 32 byte key set with
  `Csm_KeyElementSet()` gives AES-128, AES-192 or AES-256. Any other length is
  accepted into the keystore and then rejected by the job that tries to use it
  — see `csm-errors`. (Releases up to 5.9.4 pinned every cipher job to a 16
  byte key, so AES-192/256 was unreachable except through redirection.)
* **MAC length units differ by service, as the specification has them.**
  `Csm_MacGenerate()` reports its `macLengthPtr` in bytes; `Csm_MacVerify()`
  takes `macLength` in bits. Feeding the first straight into the second asks
  for an eighth of the tag, so multiply by 8 -- see `csm-threads.c`.

## Building

Build and install wolfSSL with the AUTOSAR port enabled:

```sh
$ cd wolfssl
$ ./autogen.sh   # only if configure does not exist yet
$ ./configure --enable-autosar
$ make
$ sudo make install
```

Add `--enable-autosar-cmac` for the MAC generate and verify services. It pulls
in wolfCrypt's CMAC on its own, so `--enable-cmac` is not needed separately:

```sh
$ ./configure --enable-autosar --enable-autosar-cmac
```

`csm-she-provision` additionally needs `--enable-she=standard`, and
`csm-cryptocb` needs `--enable-cryptocb`. Everything together:

```sh
$ ./configure --enable-autosar --enable-autosar-cmac \
              --enable-she=standard --enable-cryptocb
```

Examples whose feature is missing report `SKIP` and exit 0, so `make check`
passes on any of these configurations rather than failing on what you did not
build.

Or with CMake, `-DWOLFSSL_AUTOSAR=yes`. Note that releases up to and including
5.9.4 define `WOLFSSL_AUTOSAR` into `options.h` without compiling the port, so
`Csm_*` calls compile and then fail to link; use autotools with those.

Then in `wolfssl-examples/autosar`:

```sh
$ make
$ make check     # build and run every example in the directory
```

Each one checks what it was linked against, in two different ways:

* Built against a wolfSSL with no `WOLFSSL_AUTOSAR` at all, an example prints
  the `./configure` line it needs, reports `SKIP` and exits 0.
* Built against an AUTOSAR wolfSSL that is missing the option this particular
  example needs -- `--enable-autosar-cmac` for csm-secoc, `--enable-she` for
  csm-she-provision, `--enable-cryptocb` for csm-cryptocb -- it prints
  `SKIP` and exits 0, so `make check` passes on a configuration that simply
  does not have the feature.

The "what you need" line at the top of each example's section below says which
is which.

If wolfSSL is installed somewhere other than `/usr/local`:

```sh
$ make WOLFSSL_INSTALL_DIR=/opt/wolfssl
```

## csm-basic

A round trip through the CSM in the order a real SW-C would do it: bring up the
stack, pull a fresh IV from the DRBG, load the key material into the keystore,
then encrypt and decrypt.

It runs the payload two ways. `CRYPTO_OPERATIONMODE_SINGLECALL` is
`START|UPDATE|FINISH` in one call — the job fetches its key and IV from the
keystore and tears itself down immediately. The streamed version splits the
same work into `START`, `UPDATE` and `FINISH`, which is what you need when the
plaintext arrives in pieces (a multi-frame CAN payload, for instance).

The message is copied into a block-aligned buffer and zero-padded first,
because the driver will not pad for you. A real application needs a scheme it
can strip again on the far side.

Two things to watch when streaming:

* `START` allocates the job's AES context out of the `MAX_JOBS` table and
  `FINISH` releases it. Abandon a job without `FINISH` and that slot is leaked
  for the life of the ECU.
* Job state is keyed on `jobId`, so two job IDs can stream concurrently without
  interfering. Reusing one `jobId` for two interleaved streams cannot work.

```
wolfSSL AUTOSAR CSM 5.9.4

== single-call AES-128-CBC ==
plaintext:  4155544f5341522043534d2064656d6f207061796c6f61640000000000000000 (32 bytes)
iv:         ef638c993bb3b23d438f7e539e2f8a1f (16 bytes)
ciphertext: f5a9d80c42da96437a73ce7ddf713a1f9aa50cd736d33161c023c48eb6f7a8b7 (32 bytes)
recovered:  4155544f5341522043534d2064656d6f207061796c6f61640000000000000000 (32 bytes)
round trip OK

== streamed AES-128-CBC (START/UPDATE/FINISH) ==
ciphertext: a0a69e3ab0e369c8aedecab7a45f0a0bf5108db9d469dabdbc40d17509e72e5a...
round trip OK

csm-basic: PASS
```

The IV comes from the DRBG, so it and the single-call ciphertext differ every
run. The streamed example uses a fixed IV, so its ciphertext does not.

## csm-jobs

Two things about jobs that the `Csm_*` prototypes do not reveal.

**Job state is keyed on `jobId`.** The example runs two streams a block at a
time, alternating between them — job 1 UPDATE, job 2 UPDATE, job 1 UPDATE — and
checks each ciphertext against what a single call over the whole message
produces. They match, which is what lets two SW-Cs share the CSM. Both jobs
START against the same keystore key and IV here, deliberately: the CBC chain
state lives in the job, not in the key material.

The corollary: reusing one `jobId` for two interleaved streams cannot work.
A second START on a job that is already active restarts it -- the driver frees
the first context and reuses the slot -- so the first stream is silently
discarded rather than interleaved.

**The job table is `MAX_JOBS` entries and FINISH is what frees one.** The
example opens `MAX_JOBS` streams without finishing them, shows the next START
being refused, then FINISHes one and shows a new stream starting again.

```
== job table exhaustion (MAX_JOBS = 10) ==
  opened 10 streams without calling FINISH
  stream 11 refused, as expected
  after one FINISH, a new stream starts again
```

On an ECU there is no process exit to clean up after you, so a job abandoned
mid-stream — an error path that returns early without FINISH, say — costs one
of those ten slots permanently.

## csm-errors

The Csm API reports one `Std_ReturnType`, so `E_NOT_OK` is all the caller sees.
The reason goes to the Development Error Tracer, which in this port means the
wolfSSL log — build wolfSSL with `--enable-debug` and the example's
`wolfSSL_Debugging_ON()` call turns those messages on. Without it the calls
still fail correctly, just silently.

The example walks every rejection path: encrypting with an empty keystore, the
four `Csm_KeyElementSet()` argument checks (NULL key, zero length, `keyId` past
`MAX_KEYSTORE`, key too big for a slot), UPDATE and FINISH without a START, a
stored key whose length AES cannot use, and a NULL random output buffer. With
`--enable-autosar-cmac` it also covers the MAC argument checks.

Two things worth taking from it:

* **The keystore does not police key lengths, the job does.** Storing a 20 byte
  key succeeds; the `Csm_Encrypt()` that picks it up is what fails. So a key
  provisioning step that only checks `Csm_KeyElementSet()` can report success
  for a key that no job will ever be able to use.
* **A MAC mismatch is not an `E_NOT_OK`.** `Csm_MacVerify()` returns `E_OK` —
  the job ran fine — and reports the mismatch through `verifyPtr` as
  `CRYPTO_E_VER_NOT_OK`. Treating the two the same is how a genuine
  authentication failure ends up being retried as if it were transient.

```
== streaming out of order ==
  accepted  provisioning the key
  accepted  provisioning the IV
  rejected  UPDATE without START
  rejected  FINISH without START
```

## csm-secoc

AUTOSAR Secure Onboard Communication is the reason the MAC services exist. It
authenticates an I-PDU by appending a freshness value and a truncated MAC:

```
MAC input     = Data ID | Authentic I-PDU | complete Freshness Value
Secured I-PDU = Authentic I-PDU | truncated FV | truncated Authenticator
```

The example is sized so a secured PDU is exactly one classic CAN frame — no CAN
FD, no ISO-TP segmentation:

```
4 byte payload + 1 byte truncated FV + 3 byte truncated MAC = 8 bytes
```

That 24-bit authenticator is why truncation exists at all: a full 16 byte CMAC
tag does not fit in a CAN frame alongside any payload. `Csm_MacVerify()` takes a
`macLength` shorter than the tag and compares the leading bytes, which is
exactly the truncation SecOC needs.

Because only the low byte of the counter is transmitted, the verifier has to
reconstruct the complete freshness value. It walks forward from the last value
it accepted, tries each candidate whose low byte matches, and accepts the first
one whose MAC verifies. Starting at `lastFv + 1` is what makes a replay fail,
and bounding the search at `SECOC_FV_WINDOW` is what tolerates frames lost on
the bus without letting an attacker drag the counter forward.

The interesting part is which mechanism rejects what:

| Attack | Rejected by |
| --- | --- |
| Replay of an earlier genuine frame | freshness — the authenticator is still valid |
| Modified payload | the MAC |
| Modified authenticator | the MAC |
| Valid MAC, freshness far beyond the window | the window bound |
| Frame authenticated for a different Data ID | the MAC |

A verifier that checks the MAC and ignores freshness accepts every replay. That
is the classic way to get SecOC wrong, and it is why the example prints the
reason for each rejection rather than just pass or fail.

```
== attacks ==
  replayed frame:       01020304 01 0a2fa6   (payload fv mac)
  rejected  replay of an earlier genuine frame
            reason: no freshness value in the window matches the one transmitted
            note: the authenticator on this frame is still valid;
                  freshness is what rejects it
  payload flipped:      fe020312 07 99142d   (payload fv mac)
  rejected  modified payload
            reason: authenticator did not verify for any candidate freshness value
```

Each tamper case starts from a frame the verifier has not seen yet, so its
freshness value is still inside the window and the rejection genuinely comes
from the MAC. Reusing an already-accepted frame would be rejected on freshness
alone and would prove nothing about the authenticator.

### Putting it on a real bus

`secoc_authenticate()` and `secoc_verify()` are the whole SecOC layer and they
are transport agnostic: they produce and consume the 8 bytes that go in a CAN
frame's data field. The front-ends just move those bytes around, so the same
scenarios run in process or over a real interface.

With no arguments the frames never leave the process, which is why the default
run works anywhere and is what `make check` uses. On Linux, `--can` puts every
frame through a real SocketCAN interface instead:

```sh
$ sudo modprobe vcan
$ sudo ip link add dev vcan0 type vcan
$ sudo ip link set vcan0 up
$ ./csm-secoc --can vcan0
```

That opens two sockets on the one interface — SocketCAN's local loopback
delivers frames written on the first to the second — so the scenario suite runs
end to end over the bus inside a single process. Every frame the verifier sees
is one that actually crossed the interface, tampered frames included.

For two ECUs, or two terminals, there are single-role modes:

```sh
$ ./csm-secoc --recv vcan0    # terminal 1: verify until the bus goes idle
$ ./csm-secoc --send vcan0    # terminal 2: transmit genuine secured frames
```

Watch the traffic with `candump vcan0` from `can-utils`, and note that the
payload is in the clear — SecOC authenticates, it does not encrypt.

Requirements and knobs:

* Linux with the `vcan` (or a real CAN) interface. On any other platform the
  CAN options report that SocketCAN is unavailable and exit 77; the default
  in-process run is unaffected.
* `-DNO_SOCKETCAN` compiles the CAN front-ends out even on Linux.
* The receiving socket installs a kernel-space filter on `SECOC_CAN_ID`, so
  other traffic on the bus is dropped before it reaches user space.

### Limitations

* **Byte-granular truncation only.** `Csm_MacVerify()` compares whole bytes, so
  a 24-bit authenticator works but SecOC profiles specifying lengths like 28
  bits are not expressible. Real SecOC truncates at bit granularity.
* **The freshness manager is a plain counter.** A production freshness value
  comes from the SecOC freshness manager — commonly a trip counter plus a reset
  counter plus a message counter, synchronised across ECUs. The example uses a
  single monotonic counter so the freshness logic stays visible.
* **One key, compiled in.** Both ends share `secocKey`. On a vehicle this is
  provisioned per key slot. `CRYPTO_KE_MAC_KEY` and `CRYPTO_KE_CIPHER_KEY`
  share element ID `0x01`, so a build doing both cipher and MAC jobs has to
  name the slot it means -- `wolfSSL_Csm_MacGenerateWithKey()` and
  `wolfSSL_Csm_EncryptWithKey()`. Key input redirection does NOT separate them:
  it maps an element ID to one slot for every service. This example only does
  MAC jobs, so the plain call is unambiguous here.

## csm-stream

A classic CAN frame carries 8 bytes; an AES block is 16. So a single block
already spans two frames, and anything worth encrypting spans many.

The point is the memory profile. `START`/`UPDATE`/`FINISH` lets both ends work a
block at a time, so each needs 16 bytes of scratch plus its CSM job — a 1 KB
diagnostic response costs the same RAM as a 32 byte one. The example prints that
explicitly and then proves it by never allocating the message anywhere but the
test harness.

The second half is the cost of that: the stream carries no per-frame integrity,
so a lost frame is decrypted into rubbish rather than detected. How much
rubbish depends on what was lost, and the example shows both cases.

A lost CAN frame is half an AES block (8 bytes of 16), so every later block the
receiver assembles is shifted and comes out wrong. That is block alignment
being lost, not CBC chaining — ECB or CTR would fare the same:

```
== half a block lost in the middle ==
  frame 5 dropped (second half of block 2)
  7 of 8 blocks delivered, 1 never arrived
  first wrong block: 2
  every delivered block from 2 to 6 is wrong
```

A whole lost block is the CBC-specific case, and it is subtler. Decryption
needs a block and its predecessor, so the splice corrupts exactly one block and
the chain recovers at once — but nothing numbers the blocks, so everything
after it is written one block too early. The right plaintext in the wrong
place, which a positional comparison still sees as corruption:

```
== a whole block lost in the middle ==
  frames 4 and 5 dropped (all of block 2)
  7 of 8 blocks delivered, 1 never arrived
  first wrong block: 2
  the only one decrypted wrongly: CBC needs a block and
  its predecessor, so the chain recovers at once. Every
  block after it is the right plaintext, one block early
```

That is the argument for putting SecOC underneath: authenticate each frame and a
lost or altered one is detected instead of quietly decrypted into rubbish.

This is not an ISO-TP implementation — there are no FirstFrame/ConsecutiveFrame
headers and no flow control, because that is a transport protocol. The
[can-bus](../can-bus/README.md) example drives the real one through wolfSSL's
`WOLFSSL_ISOTP` support. Here the payload is segmented into CAN-sized chunks so
the crypto side is visible on its own.

## csm-threads

Four workers running jobs concurrently while a fifth thread reprovisions keys
underneath them, across single-call, streaming and MAC phases.

The driver guards the keystore and the job table with separate mutexes. What
that does and does not buy you:

* **Safe:** any number of threads running jobs at once, provided each job ID has
  one owner. Claiming and releasing job slots and reading keys are serialized
  internally.
* **Safe:** calling `Csm_KeyElementSet()` while jobs are running. The driver
  copies the key out under the keystore lock, so a job never reads a key being
  rewritten underneath it.
* **Unsafe:** two threads driving the *same* job ID. A streaming job is a
  conversation, and AUTOSAR gives each job a single owner; sharing one ID
  interleaves two sets of UPDATEs into one cipher context.

Separately, replacing key material with *different* bytes mid-stream is not a
data race but is still a logic error — the job's `START` captured the old key.
Rotate keys between messages, not during them.

`MAX_JOBS` has to be at least the number of jobs that can be streaming at once;
the example says so and refuses to run if it is too small for its worker count.

This example also serves as a regression test. Against a wolfSSL from before the
locking was added it fails on its own assertions — hundreds of `E_NOT_OK`
returns and wrong results — with no sanitizer needed.

## csm-she-provision

Every other example here calls `Csm_KeyElementSet()` with a key compiled into
the binary, which no production ECU does. This one is where the key actually
comes from.

SHE — Secure Hardware Extension — is the automotive key storage standard, and
is typically the hardware sitting behind CryIf and the Crypto driver. Its key
update protocol is five messages:

```
backend -> ECU
  M1  UID | target key ID | authorizing key ID          16 bytes
  M2  AES-CBC(K1, counter | flags | pad | new key)      32 bytes
  M3  AES-CMAC(K2, M1|M2)                               16 bytes
ECU -> backend
  M4  UID | IDs | AES-ECB(K3, counter | pad)            32 bytes
  M5  AES-CMAC(K4, M4)                                  16 bytes
```

K1/K2 derive from the authorizing key and K3/K4 from the new one, so **M3 proves
the sender knew the authorizing key and M5 proves the ECU installed the new
one**. The counter, carried inside M2 and covered by M3, is what stops an old
block being replayed.

The example is the **backend** side — a key-management service or an
end-of-line programming tool. It builds the block, works out the M4/M5 it
expects back, checks the ECU's answer, and then uses the provisioned key through
the CSM.

It pins its output against the SHE specification's memory update example (the
same M1/M4/M5 wolfSSL's own test suite pins), so a block built here is one real
SHE hardware will accept. A self-consistency check would prove nothing about
interoperability:

```
  M1   00000000000000000000000000000141
  M2   2b111e2d93f486566bcbba1d7f7a9797
       c94643b050fc5d4d7de14cff682203c3
  M3   b9d745e5ace7d41860bc63c2b9f5bb46

  M1 matches the SHE specification vector      ok
  M4 matches the SHE specification vector      ok
  M5 matches the SHE specification vector      ok
```

It then shows a wrong M5 being rejected, that a new counter changes M2, M3 and
the expected M4/M5, and that a wrong authorizing key changes M3 — so an attacker
without `MASTER_ECU_KEY` cannot forge an update.

Finally it loads the provisioned key into the CSM keystore and runs an AES-CBC
round trip and an AES-CMAC generate/verify with it. A SecOC key arrives exactly
this way.

### What it is not

**The ECU side.** Recovering the new key from M2 needs K1 derived from the
authorizing key and happens inside the HSM; wolfSSL does not expose that as a
software call, deliberately. With real hardware and a crypto callback the ECU
side is one call — `wc_SHE_LoadKey_Verify()` sends M1/M2/M3 to the HSM and
compares the M4/M5 it returns, returning `SIG_VERIFY_E` on a mismatch. That
needs `WOLF_CRYPTO_CB` and a driver for the part, so it is not exercised here.

**A demonstration of key protection.** The CSM section loads the key *value*
into the software keystore, which is the one thing a production ECU must not do.
On real hardware the key never leaves the HSM and the CSM job runs against the
slot: register a crypto callback for the part and pass its `devId` in
`Csm_ConfigType`, and the driver hands that to every context it creates. This
example has no HSM to talk to, so it says what it is doing where it happens
rather than pretending otherwise.

## csm-cryptocb

The CSM / CryIf / Crypto split exists so the bottom layer can be a real driver
for an HSM, SHE block or accelerator. In this port that happens through a
wolfCrypt crypto callback: put its device ID in `Csm_ConfigType` and every
context the driver creates carries it, so the work is offered to your callback
instead of being done in software.

This is the skeleton of that callback. There is no pretend hardware — a fake
would teach the wrong shape. What it gives you is the part that is hard to
discover: **which operations arrive for each `Csm_*` call**, and how to accept,
decline or fail them.

```
  Csm_Encrypt + Csm_Decrypt offered:
      AES-CBC encrypt  (WC_CIPHER_AES_CBC)
      AES-CBC decrypt  (WC_CIPHER_AES_CBC)
  Csm_RandomGenerate offered:
      hash             (DRBG internals)    x63
      entropy seed     (WC_ALGO_TYPE_SEED)
      random block     (WC_ALGO_TYPE_RNG)
  Csm_MacGenerate offered:
      AES-CMAC         (WC_ALGO_TYPE_CMAC) x3
```

That `x63` is worth seeing before you decide to offload: one
`Csm_RandomGenerate()` offers dozens of hash operations, because the DRBG is
built on SHA-256. A part that is slower per call than software will make random
generation worse, not better.

Three return values, and the example demonstrates all three:

* `0` — the driver handled it.
* `CRYPTOCB_UNAVAILABLE` — software does it. The fallback is transparent and
  results stay correct, so a part that does AES but not CMAC just declines the
  rest. The example switches cipher handling off mid-run to show this.
* a wolfCrypt error — the job fails, and that surfaces as `E_NOT_OK` from
  `Csm_Encrypt()`. Hardware faults are not silently ignored.

### The order matters

```c
wolfCrypt_Init();                                  /* not optional */
wc_CryptoCb_RegisterDevice(devId, callback, ctx);
config.devId = devId;
Csm_Init(&config);
```

Free slots in the crypto callback device table are marked `INVALID_DEVID`, and
nothing sets that up until `wolfCrypt_Init()` runs. Skip it and registration
fails with `BUFFER_E`, "out of devices" — which is not an obvious diagnosis.
Registration also invokes your callback with `WC_ALGO_TYPE_NONE`, which must
succeed, or registration itself fails.

### What a devId does and does not give you

It offloads the **work**. It does not move the **key**: `GetKey()` still copies
key bytes out of the software keystore and `wc_AesSetKey()` takes them, so the
key is in RAM either way. Two things change that, and the example names both:

* `WOLF_CRYPTO_CB_AES_SETKEY` offers the key install to the driver so it can
  keep the key in a hardware slot. It is a raw `-D`, not a configure option;
  build with it and the example reports the key installs it saw.
* **Naming the key instead of storing it.** With `WOLF_PRIVATE_KEY_ID`,
  `wolfSSL_Csm_KeyElementSetId()` and `wolfSSL_Csm_KeyElementSetLabel()` point a
  keystore slot at a key in the device, and the driver builds its contexts with
  `wc_AesInit_Id()` / `wc_AesInit_Label()`. The key never enters RAM. Cipher
  services only — the MAC services still need material, because wolfCrypt
  rejects a NULL key before reaching `wc_InitCmac_Id()`'s identifier path.
  Build with `-DWOLF_PRIVATE_KEY_ID` and the example says so.

## user_settings

An ECU build has no `configure` and no CMake. [`user_settings/`](user_settings/README.md)
is the port compiled straight from the wolfSSL sources with a hand-written
`user_settings.h`, reduced to the six wolfCrypt files it actually needs, with an
`ecu-app` that checks the configuration is what you think it is. Its README
documents the five things that bite — starting with `NO_FILESYSTEM` silently
removing the entropy source.

## csm-key-redirection

`Csm_KeyElementSet()` writes to a keystore slot you name, but a *job* does not
name the slot it reads. How the Crypto driver resolves that depends on how
wolfSSL was compiled:

* **Default.** The driver scans the keystore from slot 0 and takes the first
  slot whose key element ID and key length match the job. Whatever sits in the
  lowest matching slot wins.
* **Redirected.** With `REDIRECTION_CONFIG` defined, each key element ID is
  pinned to one specific slot. Other slots holding the same element ID are
  ignored.

The example loads a decoy key into slot 0 and the real key into slot 1, then
encrypts a known-answer vector and reports which key the driver actually used.

Built against a default wolfSSL, the decoy wins — which is the point:

```
built without REDIRECTION_CONFIG
  key selection is first-matching-slot

keystore slot 0: decoy key
keystore slot 1: real key

ciphertext:     9d2f0c6639de49f7a67413f6a9c18e5f
real key gives: 959492575f4281532ccc9d4677a233cb

driver used the DECOY key
```

### Turning redirection on

The redirection macros are compiled into `csm.c` and `crypto.c`, so they are a
property of **the wolfSSL library**, not of your application. Rebuild wolfSSL
with them:

```sh
$ cd wolfssl
$ ./configure --enable-autosar CPPFLAGS="\
    -DREDIRECTION_CONFIG=0x03 \
    -DREDIRECTION_IN1_KEYID=1 -DREDIRECTION_IN1_KEYELMID=0x01 \
    -DREDIRECTION_IN2_KEYID=4 -DREDIRECTION_IN2_KEYELMID=0x05"
$ make && sudo make install
```

`IN1` pins `CRYPTO_KE_CIPHER_KEY` (`0x01`) to slot 1 and `IN2` pins
`CRYPTO_KE_CIPHER_IV` (`0x05`) to slot 4. Then rebuild the example with the
same macros so it knows which behaviour to expect:

```sh
$ make clean && make REDIRECT=1
$ ./csm-key-redirection
```

Now the real key in slot 1 is used and the decoy in slot 0 is ignored:

```
built with REDIRECTION_CONFIG = 0x03
  CRYPTO_KE_CIPHER_KEY pinned to keystore slot 1
  CRYPTO_KE_CIPHER_IV  pinned to keystore slot 4
...
driver used the REAL key
Redirection pinned the job to slot 1 and ignored the decoy in slot 0.
```

The example hard-fails with a clear message if the library and the application
disagree about the slot assignment, since the alternative symptom is a silent
encryption under the wrong key.

### Caveats

* `REDIRECTION_CONFIG` itself is only tested for being defined — the bitmask
  value (`0x03` here, meaning primary and secondary input redirected) is not
  interpreted. Any non-zero value behaves the same.
* The redirection path does not check the key length against the job's
  `algorithm.keyLength`, unlike the default scan.
* Redirection is all-or-nothing per build. There is no per-job redirection at
  runtime even though `Crypto_JobType` carries a `jobRedirectionInfoRef` — the
  CSM points every job at one file-scope `redirect` struct.
* wolfSSL **5.9.4 and earlier do not compile** with `REDIRECTION_CONFIG`
  defined: `GetKey()` in `wolfcrypt/src/port/autosar/crypto.c` used runtime
  struct members as `switch` case labels, which is not valid C. It is now an
  `if`/`else if` chain. The redirected half of this example needs a wolfSSL
  that carries that fix; against an older one, `./configure` succeeds and the
  build then fails in `crypto.c`.
