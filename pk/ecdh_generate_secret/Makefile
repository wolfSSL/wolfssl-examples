CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS= -I$(WOLFSSL_INSTALL_DIR)/include -Wall
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

ecdh_gen_secret: ecdh_gen_secret.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.der *.x963 *.o ecdh_gen_secret

check: ecdh_gen_secret
	out=$$(./ecdh_gen_secret 1) && printf '%s' "$$out" | grep -q 'Successfully generated a common secret'
	out=$$(./ecdh_gen_secret 2) && printf '%s' "$$out" | grep -q 'Successfully generated a common 25519 secret'
	out=$$(./ecdh_gen_secret 3) && printf '%s' "$$out" | grep -q 'Successfully generated a common 448 secret'
	@echo "PASS: pk-ecdh-generate-secret checks"
