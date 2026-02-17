# wolfHSM Examples

Examples demonstrating wolfSSL integration with [wolfHSM](https://github.com/wolfssl/wolfhsm),
a portable Hardware Security Module (HSM) framework.

## Examples

### DTLS Client (`dtls_client/`)

DTLS client with HSM-backed cryptography. Private keys stay on the HSM.

Quick start:
```bash
cd dtls_client
make download_repos all
# Then run each component in separate terminals (see README)
```

See [dtls_client/README.md](dtls_client/README.md) for details.

## Support

For questions please email support@wolfssl.com
