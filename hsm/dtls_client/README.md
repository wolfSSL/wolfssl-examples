# wolfHSM DTLS Client Example

Demonstrates a DTLS client using wolfHSM for cryptographic operations.
All private key operations are performed on the HSM - keys never leave
the secure environment.
The example uses wolfHSM posix server for HSM communication/simulation.


## Quick Start

### Build
```bash
make download_repos   # Clone repos (first time only)
make all              # Build everything
```

### Run (3 separate terminals)

**Terminal 1** - Start wolfHSM posix server:
```bash
make run_hsm_server
```

**Terminal 2** - Start DTLS server:
```bash
make run_dtls_server
```

**Terminal 3** - Run client:
```bash
make run_client
```

## Using Existing Repositories

```bash
make WOLFSSL_DIR=/path/to/wolfssl WOLFHSM_DIR=/path/to/wolfhsm all
```

## Architecture

```
+------------------+     TCP      +------------------+
|  DTLS Client     |<------------>|  wolfHSM Server  |
|                  |              |  (crypto ops)    |
+--------+---------+              +------------------+
         | UDP/DTLS
         v
+------------------+
|  DTLS Server     |
|  (application)   |
+------------------+
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `download_repos` | Clone wolfSSL and wolfHSM |
| `all` | Build everything (default) |
| `run_hsm_server` | Start wolfHSM server (Terminal 1) |
| `run_dtls_server` | Start wolfSSL DTLS server (Terminal 2) |
| `run_client` | Run the DTLS client (Terminal 3) |
| `clean` | Clean build artifacts |
| `clean_repos` | Remove cloned repositories |
| `help` | Show help |

## Debug Build

```bash
make clean
make DEBUG=1
```
