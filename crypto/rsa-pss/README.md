# RSA PSS Example

## Building

### Build wolfSSL

```
./configure --enable-rsapss --enable-keygen --enable-certgen --enable-certext
make
sudo make install
```

### Build rsa-pss
`make`


## Usage

Creates a key-pair, exports public key to rsa-public.key and signs message in variable `szMessage`. 
`./rsa-pss -o sign.txt`

Uses public key at `rsa-public.key` to verify signed data.
`./rsa-pss -v sign.txt`
