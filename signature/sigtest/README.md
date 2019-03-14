# Signature Test Example

Demonstrates using a hash digest to sign and verify a signature using RSA

## Building

### Build and install wolfSSL

```
./configure --enable-certgen --enable-certext && make && sudo make install
```

### Build example

```
make
```

## Usage

```
./wolfsigtest
...
Signatures match!
```

```
./openssltest

...
CRYPTO: signature verify OK! 1
CRYPTO: EXPECTED signature verify OK! 1

```

### ECC Signature Length Test 

Built wolfSSL with: `./configure --enable-ecccustcurves=all && make && sudo make install`

```
./eccsiglentest README.md 
Signature Length Test: Loops 1000
File README.md is 452 bytes
ECC Curve SECP192R1, KeySz 24, Sig: ActMax 56, CalcMax 58
ECC Curve PRIME192V2, KeySz 24, Sig: ActMax 56, CalcMax 58
ECC Curve PRIME192V3, KeySz 24, Sig: ActMax 56, CalcMax 58
ECC Curve PRIME239V1, KeySz 30, Sig: ActMax 66, CalcMax 70
ECC Curve PRIME239V2, KeySz 30, Sig: ActMax 66, CalcMax 70
ECC Curve PRIME239V3, KeySz 30, Sig: ActMax 66, CalcMax 70
ECC Curve SECP256R1, KeySz 32, Sig: ActMax 72, CalcMax 74
ECC Curve SECP112R1, KeySz 14, Sig: ActMax 36, CalcMax 38
ECC Curve SECP112R2, KeySz 14, Sig: ActMax 34, CalcMax 38
ECC Curve SECP128R1, KeySz 16, Sig: ActMax 40, CalcMax 42
ECC Curve SECP128R2, KeySz 16, Sig: ActMax 38, CalcMax 42
ECC Curve SECP160R1, KeySz 20, Sig: ActMax 48, CalcMax 50
ECC Curve SECP160R2, KeySz 20, Sig: ActMax 48, CalcMax 50
ECC Curve SECP224R1, KeySz 28, Sig: ActMax 64, CalcMax 66
ECC Curve SECP384R1, KeySz 48, Sig: ActMax 104, CalcMax 106
ECC Curve SECP521R1, KeySz 66, Sig: ActMax 139, CalcMax 142
ECC Curve SECP160K1, KeySz 20, Sig: ActMax 48, CalcMax 50
ECC Curve SECP192K1, KeySz 24, Sig: ActMax 56, CalcMax 58
ECC Curve SECP224K1, KeySz 28, Sig: ActMax 64, CalcMax 66
ECC Curve SECP256K1, KeySz 32, Sig: ActMax 72, CalcMax 74
ECC Curve BRAINPOOLP160R1, KeySz 20, Sig: ActMax 48, CalcMax 50
ECC Curve BRAINPOOLP192R1, KeySz 24, Sig: ActMax 56, CalcMax 58
ECC Curve BRAINPOOLP224R1, KeySz 28, Sig: ActMax 64, CalcMax 66
ECC Curve BRAINPOOLP256R1, KeySz 32, Sig: ActMax 72, CalcMax 74
ECC Curve BRAINPOOLP320R1, KeySz 40, Sig: ActMax 88, CalcMax 90
ECC Curve BRAINPOOLP384R1, KeySz 48, Sig: ActMax 104, CalcMax 106
```

Note: The extra 2-bytes of padding is to account for the case where the base ECC key has the Most Significant Bit (MSB) set, which would cause a longer signature.
