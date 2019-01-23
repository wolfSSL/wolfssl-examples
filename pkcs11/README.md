

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
cryptoki " | pkcsconf -P -c 3

echo "cryptoki
cryptoki
cryptoki" | pkcsconf -u -c 3


# Running tests

# Start daemon if not running already
sudo /usr/local/sbin/pkcsslotd

# Change to wolfssl-examples/pkcs11 directory

./opencryptoki.sh

