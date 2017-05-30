
./ed25519_cert -c server.conf
cat server-ed25519.pem ca-ed25519.pem > server.pem
mv server.pem server-ed25519.pem

./ed25519_cert -c client.conf


