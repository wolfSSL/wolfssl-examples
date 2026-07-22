CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: certloadverifybuffer certverify certverify_ocsp

certloadverifybuffer: certloadverifybuffer.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
certverify: certverify.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
certverify_ocsp: certverify_ocsp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o certverify certloadverifybuffer certverify_ocsp

check: certverify certloadverifybuffer
	./certverify | grep -q 'CRL Verification Successful!'
	./certloadverifybuffer | grep -q 'Verification successful on cert2!'
	@echo "PASS: certmanager checks"
