# wolfSSL Post-Quantum Crypto-Proxy Examples

This directory shows two ways to use a small TLS terminating proxy to add a
**post-quantum (hybrid ML-KEM) key exchange** to a connection where one of the
two endpoints can only speak **legacy, classical TLS**. In both cases the proxy
decrypts on one side and re-encrypts on the other, so the post-quantum
protection is added transparently and neither legacy endpoint has to change.

### Direction 1 — quantum-safe front door (`pq-proxy`)

A modern client reaches a legacy origin that cannot be upgraded. The
client-facing leg is post-quantum; the origin-facing leg stays classical.

```
  ┌────────────┐  PQC / hybrid TLS 1.3   ┌────────────┐  legacy TLS 1.2   ┌───────────────┐
  │ pq-client  │ ──────────────────────► │  pq-proxy  │ ────────────────► │ legacy-server │
  │ (modern)   │   P256_ML_KEM_768       │ (terminates│  ECDHE-RSA-AES128 │  (origin)     │
  └────────────┘                         │  + re-TLS) │  -GCM-SHA256       └───────────────┘
                                         └────────────┘
```

### Direction 2 — quantum-safe upgrade (`upgrade-proxy`)

A legacy client that cannot speak PQC reaches a modern origin. The proxy
upgrades the connection so the leg crossing the untrusted network to the server
is post-quantum.

```
  ┌───────────────┐  legacy TLS 1.2   ┌──────────────┐  PQC / hybrid TLS 1.3   ┌───────────┐
  │ legacy-client │ ────────────────► │ upgrade-proxy│ ──────────────────────► │ pq-server │
  │ (legacy)      │  ECDHE-RSA-AES128 │ (terminates  │   P256_ML_KEM_768       │ (modern)  │
  └───────────────┘  -GCM-SHA256      │  + re-TLS)   │                         └───────────┘
                                      └──────────────┘
```

Each proxy is simultaneously a TLS **server** on one leg and a TLS **client**
on the other.

## Why this is useful

A "harvest now, decrypt later" attacker can record TLS traffic today and break
the (classical) key exchange once a quantum computer exists. Putting a
post-quantum **hybrid** key exchange on the leg that crosses the public network
removes that risk, even when one of the endpoints behind the proxy cannot yet
be upgraded.

## Files

| File | Role | Crypto |
|------|------|--------|
| `pq-proxy.c`      | Direction 1 proxy: PQC frontend, legacy backend | frontend: TLS 1.3 hybrid ML-KEM, backend: TLS 1.2 ECDHE-RSA |
| `legacy-server.c` | Direction 1 legacy origin                       | TLS 1.2 ECDHE-RSA-AES128-GCM-SHA256 |
| `pq-client.c`     | Direction 1 modern client                       | TLS 1.3 hybrid ML-KEM |
| `upgrade-proxy.c` | Direction 2 proxy: legacy frontend, PQC backend | frontend: TLS 1.2 ECDHE-RSA, backend: TLS 1.3 hybrid ML-KEM |
| `pq-server.c`     | Direction 2 modern origin                       | TLS 1.3 hybrid ML-KEM |
| `legacy-client.c` | Direction 2 legacy client                       | TLS 1.2 ECDHE-RSA-AES128-GCM-SHA256 |
| `proxy-common.h`  | Shared ports, groups, cert paths                | — |

## Prerequisites

Build and install wolfSSL with ML-KEM (Kyber) and TLS 1.3:

```sh
./configure --enable-mlkem --enable-tls13
make
sudo make install
sudo ldconfig
```

If you also build with `--enable-mldsa`, the PQC leg automatically upgrades to a
**fully** post-quantum handshake (ML-DSA certificate + ML-KEM key exchange).
Otherwise the PQC leg uses post-quantum key exchange with a classical RSA
certificate for authentication — which is the most common real-world PQC
deployment today, because public CAs do not yet issue PQC certificates.

## Building

```sh
make
```

## Running direction 1 (quantum-safe front door)

Use three terminals.

1. Start the legacy origin server (classical TLS 1.2):

   ```sh
   ./legacy-server
   ```

2. Start the crypto-proxy:

   ```sh
   ./pq-proxy
   ```

   ```
   PQC crypto-proxy ready
     frontend : 0.0.0.0:11111  TLS 1.3  kex=P256_ML_KEM_768  auth=RSA-2048 (classical)
     backend  : 127.0.0.1:11112  TLS 1.2  ECDHE-RSA-AES128-GCM-SHA256
   ```

3. Connect with the modern client and type a message:

   ```sh
   ./pq-client 127.0.0.1
   ```

   ```
   Connected to proxy: TLSv1.3 TLS_AES_256_GCM_SHA384  kex=SecP256r1MLKEM768
   Message for server: hello
   Server: [origin] legacy service reached over TLS 1.2
   ```

The proxy reports both halves of the bridge:

```
Bridging connection:
  frontend  TLSv1.3  TLS_AES_256_GCM_SHA384  kex=SecP256r1MLKEM768
  backend   TLSv1.2  TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256  kex=SECP256R1
```

## Running direction 2 (quantum-safe upgrade)

Use three terminals.

1. Start the modern PQC origin server (TLS 1.3, hybrid ML-KEM):

   ```sh
   ./pq-server
   ```

2. Start the upgrade-proxy:

   ```sh
   ./upgrade-proxy
   ```

   ```
   PQC upgrade-proxy ready
     frontend : 0.0.0.0:22221  TLS 1.2  ECDHE-RSA-AES128-GCM-SHA256
     backend  : 127.0.0.1:22222  TLS 1.3  kex=P256_ML_KEM_768  auth=RSA-2048 (classical)
   ```

3. Connect with the legacy client and type a message:

   ```sh
   ./legacy-client 127.0.0.1
   ```

   ```
   Connected to proxy: TLSv1.2 TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
   Message for server: hello
   Server: [pq-server] reached over a quantum-safe TLS 1.3 link
   ```

The proxy reports both halves of the bridge:

```
Bridging connection:
  frontend  TLSv1.2  TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256  kex=SECP256R1
  backend   TLSv1.3  TLS_AES_256_GCM_SHA384  kex=SecP256r1MLKEM768
```

In either direction, send the message `shutdown` to stop the origin server.

You can verify a proxy really requires PQC on its quantum-safe leg: it only ever
negotiates the `P256_ML_KEM_768` group there (via `wolfSSL_CTX_set_groups()`),
so a classical-only peer on that leg is rejected.

## Command-line options

```sh
./legacy-server [<listen port>]
./pq-proxy      [<frontend port> [<backend host> [<backend port>]]]
./pq-client     [<proxy host> [<proxy port>]]

./pq-server     [<listen port>]
./upgrade-proxy [<frontend port> [<backend host> [<backend port>]]]
./legacy-client [<proxy host> [<proxy port>]]
```

The two directions use different default ports (11111/11112 and 22221/22222),
so both demos can run at the same time.

## Customising the cryptography

All algorithm selection lives in `proxy-common.h`:

- **PQC group** — change `FRONTEND_GROUP` to raise the security level
  (e.g. `WOLFSSL_SECP384R1MLKEM1024`) or to use a non-hybrid, pure-PQC group
  (`WOLFSSL_ML_KEM_768`). Both proxies restrict negotiation on their PQC leg to
  this single group, so only quantum-safe peers are accepted there.
- **Legacy cipher suite** — change `BACKEND_CIPHERS` to model whatever legacy
  suite your real endpoint requires.

## Notes and limitations

- For clarity each proxy handles **one client connection at a time**. A
  production proxy would `fork()` or spawn a thread per accepted connection (or
  use the non-blocking / event-loop patterns in `tls/server-tls-epoll-*.c`),
  reusing the two `WOLFSSL_CTX` objects across connections as these examples
  already do.
- Data is relayed with a `select()` loop over the two socket descriptors,
  draining any records wolfSSL has already buffered with `wolfSSL_pending()`.
- The example certificates come from the repository's `certs/` directory and
  are for testing only.
