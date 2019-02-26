# RSA PSS Example

## Building

### Build wolfSSL

```
./configure --enable-rsapss --enable-keygen
make
sudo make install
```

### Build rsa-pss
`make`


## Usage

Creates a key-pair, exports public key to `rsa-public.der` and signs message in variable `szMessage`. 
`./rsa-pss -s sign.txt`

Uses public key at `rsa-public.der` to verify signed data.
`./rsa-pss -v sign.txt`
