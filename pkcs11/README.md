

### Setting up and testing SoftHSM version 2

# Change to source code directory of SoftHSM version 2

sh ./autogen.sh
# (May need to install pkg-config and libssl-dev)

./configure --disable-gost

sudo make install


# Change to wolfssl-examples/pkcs11 directory

./mksofthsm2_conf.sh

export SOFTHSM2_CONF=$PWD/softhsm2.conf


# Running tests

softhsm2-util --init-token --slot 0 --label SoftToken
# Use PIN: cryptoki
# Use User PIN: cryptoki

# Use the slot id from the output

export SOFTHSM2_SLOTID=<slotid>

./softhsm2.sh



### Setting up and testing openCryptoki


# Change to source code directory of openCryptoki

./bootstrap.sh

./configure
# (May need to install flex, bison and openldap-devel [or libldap2-dev].)

make

# Setup pkcs11 group and put current user into it
sudo groupadd pkcs11
sudo usermod -a -G pkcs11 $USER

# Install library
sudo make install

sudo ldconfig /usr/local/lib

# Start the daemon
sudo /usr/local/sbin/pkcsslotd

# (May need to logout and login to be able to use pkcsconf)

# Setup token
echo "87654321
SoftToken" | pkcsconf -I -c 3

echo "87654321
cryptoki
cryptoki" | pkcsconf -P -c 3

echo "cryptoki
cryptoki
cryptoki" | pkcsconf -u -c 3


# Running tests

# Start daemon if not running already
sudo /usr/local/sbin/pkcsslotd

# Change to wolfssl-examples/pkcs11 directory

./opencryptoki.sh



### Server TLS PKCS #11

The example server-tls-pkcs11 is a server that uses a private key that has been
stored on the PKCS #11 device.

The id of the private key is two hex bytes: 0x00, 0x01

Change this to be the id that you set when importing the key.

## SoftHSM version 2

# Import private key

softhsm2-util --import ../certs/server-keyPkcs8.pem --slot $SOFTHSM2_SLOTID --id 0001 --label rsa2048

## Run server and client

### Build wolfSSL

./autogen.sh
./configure --enable-pkcs11
make
sudo make install

### Build Server Example

make
./server-tls-pkcs11 <Path to SoftHSMv2 library> $SOFTHSM2_SLOTID SoftToken cryptoki

### Run Client
./examples/client/client


