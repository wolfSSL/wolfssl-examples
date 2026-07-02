# RTL8735B (AmebaPro2) HUK-in-TLS example

In-memory TLS 1.2 ECDHE-ECDSA handshake (client + server in one firmware, no
networking) where the server's certificate ECDSA P-256 signature is produced on
the RTL8735B HW ECDSA engine via the wolfCrypt HUK crypto-callback port. The
ECDHE key agreement and record-layer AES-GCM run in software. Proves
HUK/HW-accelerated server authentication in a real TLS handshake.

Validated on RTL8735B silicon:

```
== TLS 1.2 ECDHE-ECDSA (server auth on HUK) ==
[PASS] TLS handshake completed
cipher: ECDHE-ECDSA-AES128-GCM-SHA256
[PASS] client write
[PASS] server read == client msg
```

## Key design point: use a PK sign callback, NOT a whole-CTX devId

Do NOT `wolfSSL_CTX_SetDevId(ctx, WC_HUK_DEVID)` for a TLS CTX. The HUK device
treats a 32-byte HMAC/AES key as a *HUK seed* (HKDF input), but TLS legitimately
uses 32-byte keys in its own crypto -- e.g. the P-256 ECDHE pre-master secret is
32 bytes and TLS 1.2's PRF computes `master_secret = HMAC-SHA256(pre_master,...)`
with it. Routing that PRF HMAC to the HUK derives the wrong master secret ->
wrong session keys -> the peer's Finished fails (`VERIFY_MAC_ERROR`, -305).

Instead, route ONLY the server's certificate signature to the HUK with a PK ECC
sign callback (`wolfSSL_CTX_SetEccSignCb`). Inside it, sign with a temporary
`ecc_key` whose `devId = WC_HUK_DEVID`, so only that ECDSA sign dispatches to the
HW engine while the PRF/HMAC, record AES, and ECDHE stay in software. See
`huk_ecc_sign_cb` in `main.c`. Requires `HAVE_PK_CALLBACKS`.

## Build

Install this directory at `<SDK>/component/example/wolfcrypt_huk_tls/` and select
it in the RealTek AmebaPro2 FreeRTOS SDK build:

```
cmake .. -DEXAMPLE=wolfcrypt_huk_tls -DWOLFSSL_ROOT=/path/to/wolfssl ...
```

(also copy `main.c` to `<SDK>/project/realtek_amebapro2_v0_example/src/main.c`).
See the `amebapro2-flash` build/flash flow used by the sibling `wolfcrypt_huk`
example.

## Notes

- P-256 test certificates come from wolfSSL's `certs_test.h` (`USE_CERT_BUFFERS_256`:
  `serv_ecc_der_256`, `ecc_key_der_256`, `ca_ecc_cert_der_256`).
- `NO_ASN_TIME` is set (the board has no RTC, so certificate date checks are
  skipped) -- do not use that in production without a real time source.
- `WOLFSSL_USER_IO` + `WOLFSSL_NO_SOCK`: the transport is two in-memory buffers
  via custom `SetIORecv`/`SetIOSend` callbacks; no sockets are used.
- Model shown is the general HW-offload key (a plain P-256 server key signed on
  the engine). A device-bound HUK-wrapped server key is a follow-on (the sign
  callback would use a `wc_Rtl8735b_EccKey` via the key's `devCtx`).
