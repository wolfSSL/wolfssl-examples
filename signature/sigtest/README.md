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
./opensigtest

...
CRYPTO: signature verify OK! 1
CRYPTO: EXPECTED signature verify OK! 1

```

### ECC Signature Length Test 

Built wolfSSL with: `./configure --enable-ecccustcurves=all && make && sudo make install`

CurveMax = wc_ecc_sig_size(key)
ActMax = wc_ecc_sign_hash()
CalcMax = wc_ecc_sig_size_calc()

```
./eccsiglentest Makefile 
Signature Length Test: Loops 1000
File Makefile is 658 bytes
ECC Curve SECP192R1, KeySz 24, Sig: CurveMax 56, ActMax 56, CalcMax 56
ECC Curve PRIME192V2, KeySz 24, Sig: CurveMax 56, ActMax 56, CalcMax 56
ECC Curve PRIME192V3, KeySz 24, Sig: CurveMax 56, ActMax 56, CalcMax 56
ECC Curve PRIME239V1, KeySz 30, Sig: CurveMax 66, ActMax 66, CalcMax 68
ECC Curve PRIME239V2, KeySz 30, Sig: CurveMax 66, ActMax 66, CalcMax 68
ECC Curve PRIME239V3, KeySz 30, Sig: CurveMax 66, ActMax 66, CalcMax 68
ECC Curve SECP256R1, KeySz 32, Sig: CurveMax 72, ActMax 72, CalcMax 72
ECC Curve SECP112R1, KeySz 14, Sig: CurveMax 36, ActMax 36, CalcMax 36
ECC Curve SECP112R2, KeySz 14, Sig: CurveMax 34, ActMax 34, CalcMax 36
ECC Curve SECP128R1, KeySz 16, Sig: CurveMax 40, ActMax 40, CalcMax 40
ECC Curve SECP128R2, KeySz 16, Sig: CurveMax 38, ActMax 38, CalcMax 40
ECC Curve SECP160R1, KeySz 20, Sig: CurveMax 46, ActMax 46, CalcMax 48
ECC Curve SECP160R2, KeySz 20, Sig: CurveMax 46, ActMax 46, CalcMax 48
ECC Curve SECP224R1, KeySz 28, Sig: CurveMax 64, ActMax 64, CalcMax 64
ECC Curve SECP384R1, KeySz 48, Sig: CurveMax 104, ActMax 104, CalcMax 104
ECC Curve SECP521R1, KeySz 66, Sig: CurveMax 139, ActMax 139, CalcMax 141
ECC Curve SECP160K1, KeySz 20, Sig: CurveMax 46, ActMax 46, CalcMax 48
ECC Curve SECP192K1, KeySz 24, Sig: CurveMax 56, ActMax 56, CalcMax 56
ECC Curve SECP224K1, KeySz 28, Sig: CurveMax 62, ActMax 62, CalcMax 64
ECC Curve SECP256K1, KeySz 32, Sig: CurveMax 72, ActMax 72, CalcMax 72
ECC Curve BRAINPOOLP160R1, KeySz 20, Sig: CurveMax 48, ActMax 48, CalcMax 48
ECC Curve BRAINPOOLP192R1, KeySz 24, Sig: CurveMax 56, ActMax 56, CalcMax 56
ECC Curve BRAINPOOLP224R1, KeySz 28, Sig: CurveMax 64, ActMax 64, CalcMax 64
ECC Curve BRAINPOOLP256R1, KeySz 32, Sig: CurveMax 72, ActMax 72, CalcMax 72
ECC Curve BRAINPOOLP320R1, KeySz 40, Sig: CurveMax 88, ActMax 88, CalcMax 88
ECC Curve BRAINPOOLP384R1, KeySz 48, Sig: CurveMax 104, ActMax 104, CalcMax 104
```

Note: The extra 2-bytes of padding is to account for the case where R or S has the Most Significant Bit (MSB) set.

Please contact support@wolfssl.com with any questions or concerns!
