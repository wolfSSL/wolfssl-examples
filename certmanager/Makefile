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

.PHONY: clean

clean:
	rm -f *.o certverify certloadverifybuffer certverify_ocsp
