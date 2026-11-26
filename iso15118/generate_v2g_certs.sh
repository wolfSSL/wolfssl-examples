#!/bin/sh
# Builds a V2G certificate hierarchy of the shape ISO 15118 defines.
#
#   V2G Root CA
#     +-- CPO Sub-CA 1 -- CPO Sub-CA 2 -- SECC leaf         (the charger)
#     +-- MO  Sub-CA 1 -- MO  Sub-CA 2 -- Contract leaf     (the vehicle)
#     +-- OEM Sub-CA 1 -- OEM Sub-CA 2 -- Provisioning leaf (the vehicle, as
#                                                            built)
#
# The OEM provisioning certificate is installed at manufacture and is what the
# vehicle proves itself with when it asks for a contract certificate. See
# contract-install.c.
#
# Four tiers from root to leaf on each side, which is the depth the standard
# allows. Both sides chain to the same V2G Root CA, so each end can verify the
# other against one trust anchor.
#
# Everything is secp256r1 with SHA-256, as ISO 15118 requires. Validity
# periods differ per tier on purpose: a root outlives the sub-CAs, which
# outlive the leaves.
#
# These are test credentials. Never ship them.

set -e

# Work in the script's own directory, whatever directory it was called from.
# Without this the rm below resolves "certs" against the caller's cwd: run as
# `sh iso15118/generate_v2g_certs.sh` from the repository root, it would delete
# the repository's own tracked certs/ -- which doip and most other examples
# load through ../certs/ -- and write the V2G hierarchy in its place.
cd "$(dirname "$0")"

CURVE=prime256v1
DIR=certs
rm -rf "$DIR"
mkdir -p "$DIR"
cd "$DIR"

# openssl needs an extensions file per issued tier. The self-signed root uses
# -addext instead, because 'req -x509' does not take -extfile.
# Sub-CAs are depth limited: a sub-CA 2 must not issue another CA.
cat > subca1.ext <<'EOF'
basicConstraints = critical, CA:TRUE, pathlen:1
keyUsage         = critical, digitalSignature, keyCertSign, cRLSign
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
EOF

cat > subca2.ext <<'EOF'
basicConstraints = critical, CA:TRUE, pathlen:0
keyUsage         = critical, digitalSignature, keyCertSign, cRLSign
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
EOF

# The SECC identifies the charging station. TLS server auth.
cat > secc.ext <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage         = critical, digitalSignature, keyAgreement
extendedKeyUsage = serverAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
subjectAltName   = DNS:secc.charge.example
EOF

# The contract certificate identifies the vehicle's charging contract. It is
# what the EVCC presents for TLS client authentication in -20, and what signs
# authorisation messages.
cat > contract.ext <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage         = critical, digitalSignature
extendedKeyUsage = clientAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
EOF

# The OEM provisioning certificate. Its key is used for ECDH key agreement
# during contract certificate installation, not for TLS, hence keyAgreement
# rather than clientAuth.
cat > provisioning.ext <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage         = critical, digitalSignature, keyAgreement
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
EOF

newkey() {
    openssl ecparam -name "$CURVE" -genkey -noout -out "$1.key" 2>/dev/null
}

# self_signed <name> <subject> <days>
# 'req -x509' takes -addext, not -extfile; -extfile belongs to 'x509 -req'.
self_signed() {
    openssl req -new -x509 -sha256 -key "$1.key" -out "$1.pem" \
        -days "$3" -subj "$2" \
        -addext "basicConstraints=critical,CA:TRUE" \
        -addext "keyUsage=critical,digitalSignature,keyCertSign,cRLSign" \
        -addext "subjectKeyIdentifier=hash" 2>/dev/null
}

# issue <name> <subject> <issuer> <days> <extfile>
issue() {
    openssl req -new -sha256 -key "$1.key" -subj "$2" -out "$1.csr" 2>/dev/null
    openssl x509 -req -sha256 -in "$1.csr" -CA "$3.pem" -CAkey "$3.key" \
        -CAcreateserial -out "$1.pem" -days "$4" \
        -extfile "$5" 2>/dev/null
    rm -f "$1.csr"
}

echo "V2G Root CA"
newkey v2g-root
self_signed v2g-root \
    "/C=DE/O=V2G/OU=V2G Root CA/CN=V2G Root CA" 3650

echo "CPO chain (charging station)"
newkey cpo-subca1
issue cpo-subca1 "/C=DE/O=CPO/OU=CPO Sub-CA 1/CN=CPO Sub-CA 1" \
    v2g-root 1825 subca1.ext
newkey cpo-subca2
issue cpo-subca2 "/C=DE/O=CPO/OU=CPO Sub-CA 2/CN=CPO Sub-CA 2" \
    cpo-subca1 1095 subca2.ext
newkey secc
issue secc "/C=DE/O=CPO/OU=SECC/CN=secc.charge.example" \
    cpo-subca2 60 secc.ext

echo "MO chain (vehicle contract)"
newkey mo-subca1
issue mo-subca1 "/C=DE/O=MO/OU=MO Sub-CA 1/CN=MO Sub-CA 1" \
    v2g-root 1825 subca1.ext
newkey mo-subca2
issue mo-subca2 "/C=DE/O=MO/OU=MO Sub-CA 2/CN=MO Sub-CA 2" \
    mo-subca1 1095 subca2.ext
newkey contract
# The contract certificate's CN carries the e-mobility account identifier
issue contract "/C=DE/O=MO/OU=Contract/CN=DE-MO-C0123456789-3" \
    mo-subca2 730 contract.ext

echo "OEM chain (vehicle as built)"
newkey oem-subca1
issue oem-subca1 "/C=DE/O=OEM/OU=OEM Sub-CA 1/CN=OEM Sub-CA 1" \
    v2g-root 1825 subca1.ext
newkey oem-subca2
issue oem-subca2 "/C=DE/O=OEM/OU=OEM Sub-CA 2/CN=OEM Sub-CA 2" \
    oem-subca1 1095 subca2.ext
newkey provisioning
# The provisioning certificate's CN is the PCID, tied to the vehicle
# 1000 days, inside OEM Sub-CA 2's 1095: a certificate outliving its issuer is
# exactly the mistake this hierarchy is meant not to make. A real OEM
# provisioning identity is meant to last the life of the vehicle, which is
# handled by reissuing the sub-CA, not by overrunning it.
issue provisioning "/C=DE/O=OEM/OU=Provisioning/CN=WOLFSSL0000000001" \
    oem-subca2 1000 provisioning.ext

# A second provisioning identity, for showing that only the vehicle holding
# the right private key can install the contract.
newkey other-provisioning
issue other-provisioning \
    "/C=DE/O=OEM/OU=Provisioning/CN=WOLFSSL0000000002" \
    oem-subca2 1000 provisioning.ext

# Chain files: leaf first, then the sub-CAs, root excluded. The root is a
# locally configured trust anchor and is not sent on the wire -- which is why
# each side transmits three certificates, not four.
cat secc.pem cpo-subca2.pem cpo-subca1.pem > secc-chain.pem
cat contract.pem mo-subca2.pem mo-subca1.pem > contract-chain.pem
cat provisioning.pem oem-subca2.pem oem-subca1.pem > provisioning-chain.pem

rm -f *.ext *.srl

echo
echo "verifying all three chains against the V2G root"
openssl verify -CAfile v2g-root.pem -untrusted cpo-subca1.pem \
    -untrusted cpo-subca2.pem secc.pem
openssl verify -CAfile v2g-root.pem -untrusted mo-subca1.pem \
    -untrusted mo-subca2.pem contract.pem
openssl verify -CAfile v2g-root.pem -untrusted oem-subca1.pem \
    -untrusted oem-subca2.pem provisioning.pem

echo
echo "hierarchy in $DIR/ -- 3 branches, 4 tiers each, 3 certs on the wire"
