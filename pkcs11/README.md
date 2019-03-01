# Examples for PKCS #11 with wolfSSL

These examples demonstrate using wolfSSL's PKCS #11 feature for the following algorithms:

* ECC Key Gen, Sign/Verify and ECDHE (Shared Secret)
* RSA Key Gen and Sign/Verify
* AES GCM

This also includes a TLS server example using a PKCS 11 based key.

## API Reference

See [PKCS11.md](./PKCS11.md) in this folder.

## Setting up and testing SoftHSM version 2

1. Change to source code directory of SoftHSM version 2

	This tool can be found here: https://github.com/opendnssec/SoftHSMv2

	```
	./autogen.sh
 	./configure --disable-gost
 	sudo make install
 	```

 	*Note: May need to install pkg-config and libssl-dev*

2. Change to wolfssl directory

	```
	./autogen.sh
	./configure --enable-pkcs11
	make
	sudo make install
	```

3. Change to wolfssl-examples/pkcs11 directory

	```
	./mksofthsm2_conf.sh
	export SOFTHSM2_CONF=$PWD/softhsm2.conf
	```

4. Running tests

	`softhsm2-util --init-token --slot 0 --label SoftToken`

	* Use PIN: cryptoki
	* Use User PIN: cryptoki

	Use the slot id from the output:
	
	`export SOFTHSM2_SLOTID=<slotid>`

	Run the examples:

	`./softhsm2.sh`


## Setting up and testing openCryptoki


1. Change to source code directory of openCryptoki

	This tool can be found here: https://github.com/opencryptoki/opencryptoki

	```
	./bootstrap.sh
	./configure
	make
	```

	*Note: May need to install flex, bison and openldap-devel [or libldap2-dev]*

2. Setup pkcs11 group and put current user into it

	```
	sudo groupadd pkcs11
	sudo usermod -a -G pkcs11 $USER
	```

3. Install library

	```
	sudo make install
	sudo ldconfig /usr/local/lib
	```

4. Start the daemon

	`sudo /usr/local/sbin/pkcsslotd`
	
	*Note: May need to logout and login to be able to use pkcsconf.*

5. Setup token
	
	```
	echo "87654321
	SoftToken" | pkcsconf -I -c 3
	```
	
	```
	echo "87654321
	cryptoki
	cryptoki" | pkcsconf -P -c 3
	```
	
	```
	echo "cryptoki
	cryptoki
	cryptoki" | pkcsconf -u -c 3
	```
	
6. Start daemon if not running already:
	
	`sudo /usr/local/sbin/pkcsslotd`

7. Build and install wolfSSL

	Change to wolfssl directory and run:

	```
	./autogen.sh
	./configure --enable-pkcs11
	make
	sudo make install
	```


8. Running tests

	Change to wolfssl-examples/pkcs11 directory:
	
	`./opencryptoki.sh`


## TLS Server Example with PKCS #11 (RSA)

The example `server-tls-pkcs11` is a server that uses a private key that has been stored on the PKCS #11 device.

The id of the private key is two hex bytes: `0x00, 0x01`

Change this to be the id that you set when importing the key.

1. SoftHSM version 2

	Import private key:
	
	`softhsm2-util --import ../certs/server-keyPkcs8.pem --slot $SOFTHSM2_SLOTID --id 0001 --label rsa2048`

	Enter PIN: cryptoki

2. Run server and client

	`./server-tls-pkcs11 /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki`

	From wolfssl root:
	`./examples/client/client`

## TLS Server Example with PKCS #11 (ECC)

The example `server-tls-pkcs11-ecc` is a server that uses a private key that has been stored on the PKCS #11 device.

The id of the private key is two hex bytes: `0x00, 0x01`

Change this to be the id that you set when importing the key.

1. SoftHSM version 2

	Import private key:
	
	`softhsm2-util --import ../certs/ecc-keyPkcs8.pem --slot $SOFTHSM2_SLOTID --id 0002 --label ecp256`

	Enter PIN: cryptoki

2. Run server and client

	`./server-tls-pkcs11-ecc /usr/local/lib/softhsm/libsofthsm2.so $SOFTHSM2_SLOTID SoftToken cryptoki`

	From wolfssl root:
	`./examples/client/client -A ./certs/ca-ecc-cert.pem`


## Support

For questions please contact wolfSSL support by email at [support@wolfssl.com](mailto:support@wolfssl.com)
