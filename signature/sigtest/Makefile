CC=gcc
CFLAGS=-Wall
LIBS=

# set OPENSSL_DIR only for a non-system OpenSSL (e.g. Homebrew: /usr/local/opt/openssl)
OPENSSL_DIR ?=
ifneq ($(OPENSSL_DIR),)
OPENSSL_FLAGS=-I$(OPENSSL_DIR)/include
OPENSSL_LIB=-L$(OPENSSL_DIR)/lib -lcrypto -lssl
else
OPENSSL_FLAGS=
OPENSSL_LIB=-lcrypto -lssl
endif

WOLFSSL_INSTALL_DIR = /usr/local
WOLFSSL_FLAGS=-I$(WOLFSSL_INSTALL_DIR)/include
WOLFSSL_LIB= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:wolfsigtest opensigtest eccsiglentest

opensigtest:CFLAGS+=$(OPENSSL_FLAGS)
opensigtest:opensigtest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(OPENSSL_LIB) 

wolfsigtest:CFLAGS+=$(WOLFSSL_FLAGS)
wolfsigtest:wolfsigtest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(WOLFSSL_LIB) 

eccsiglentest:CFLAGS+=$(WOLFSSL_FLAGS)
eccsiglentest:eccsiglentest.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS) $(WOLFSSL_LIB)

.PHONY: clean all

clean:
	rm -f *.o wolfsigtest opensigtest eccsiglentest
