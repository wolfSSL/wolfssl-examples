CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

ecdh_gen_secret: ecdh_gen_secret.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.der *.x963 *.o ecdh_gen_secret
