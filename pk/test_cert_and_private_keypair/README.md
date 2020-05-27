An app to quickly check if a cert and private key are a pair.

Run with `test-cert-privkey-pair yourkey.pem yourcert.pem` to test your key and
cert pair.

NOTE: Load key first then cert.


EXAMPLE 1: Improper load
./test-cert-privkey-pair ~/Downloads/wolfssl/certs/server-ecc-rsa.pem ~/Downloads/wolfssl/certs/server-cert.pem
Failed to load PRIVATE KEY /Users/kalebhimes/clean-wolfssl/certs/server-ecc-rsa.pem, error was: 0

EXAMPLE 2: Correct load, not a pair
./test-cert-privkey-pair ~/Downloads/wolfssl/certs/client-key.pem ~/Downloads/wolfssl/certs/server-cert.pem
This key and cert are not a pair
Error: 0

EXAMPLE 3: Correct load, is a pair
./test-cert-privkey-pair ~/Downloads/wolfssl/certs/client-key.pem ~/Downloads/wolfssl/certs/client-cert.pem
Congratulations, this private key and cert are a pair!
