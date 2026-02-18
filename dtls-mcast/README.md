# DTLS 1.2 Multicast Example

This example demonstrates DTLS 1.2 multicast communication using wolfSSL's multicast APIs. Three peer applications can securely communicate with each other over UDP multicast.

## Overview

DTLS multicast allows multiple peers to share encrypted/authenticated communication over IP multicast. Unlike traditional TLS/DTLS which uses a handshake to establish keys, multicast DTLS uses pre-shared secrets that are distributed out-of-band to all participants.

This example uses:
- **WDM-NULL-SHA256** cipher suite (NULL encryption with SHA-256 for message authentication)
- Pre-shared secret material (PMS, client random, server random)
- Multicast group `239.255.0.1:12345`

NOTE: Since this is using NULL encryption, all messages going over the wire are plaintext. This example provides no confidentiality.

## Requirements

wolfSSL must be built with multicast support:

```bash
cd /path/to/wolfssl
./configure --enable-dtls --enable-mcast
make
```

## Building

```bash
make
```
## Usage

Run each peer in a separate terminal with a unique node ID (0, 1, or 2):

```bash
# Terminal 1
./mcast-peer 0

# Terminal 2
./mcast-peer 1

# Terminal 3
./mcast-peer 2
```

Each peer will:
1. Join the multicast group
2. Send a message every 3 seconds
3. Receive and display messages from other peers
4. Exit cleanly on Ctrl+C

## Example Output

```
=== DTLS Multicast Peer - Node 0 ===
Node 0: Sockets ready, joined multicast group 239.255.0.1:12345
Node 0: Added peer 1 to receive tracking
Node 0: Added peer 2 to receive tracking
Node 0: Ready. Press Ctrl+C to exit.
Node 0: Sending messages every 3 seconds...

Node 0: Sent: "Hello from node 0, message #1"
Node 0: Received from peer 1: "Hello from node 1, message #1"
Node 0: Received from peer 2: "Hello from node 2, message #1"
```

## API Usage Notes

The wolfSSL multicast APIs must be called in a specific order:

1. `wolfSSL_CTX_mcast_set_member_id()` - Set this node's ID
2. `wolfSSL_CTX_set_cipher_list()` - Set multicast cipher suite
3. `wolfSSL_new()` - Create SSL objects
4. `wolfSSL_mcast_peer_add()` - Register expected peers (**before** setting secret)
5. `wolfSSL_set_secret()` - Set the pre-shared secret (**after** adding peers)
6. `wolfSSL_write()` / `wolfSSL_mcast_read()` - Send/receive messages

**Important**: `wolfSSL_mcast_peer_add()` must be called BEFORE `wolfSSL_set_secret()` because `wolfSSL_mcast_peer_add()` zeros the peer entry (including the epoch), and `wolfSSL_set_secret()` sets the epoch for all registered peers.

## Key Concepts

### Multicast Member ID
Each peer has a unique ID (0-255) set via `wolfSSL_CTX_mcast_set_member_id()`. This ID is embedded in outgoing DTLS records and used by receivers to identify the sender.

### Pre-Shared Secret
All peers must use identical secret material:
- Pre-master secret (PMS)
- Client random
- Server random
- Cipher suite identifier

In production, this material would be distributed securely out-of-band (e.g., via a key server).

### NULL Encryption
This example is using NULL encryption. That means all messages going over the wire are plaintext. This example provides no confidentiality.

## References

- [wolfSSL Manual - DTLS](https://www.wolfssl.com/documentation/manuals/wolfssl/chapter02.html)
- [RFC 6347 - DTLS 1.2](https://tools.ietf.org/html/rfc6347)
