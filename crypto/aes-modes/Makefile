CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm

# All AES mode examples
EXAMPLES = aes-cbc aes-cfb aes-cfb1 aes-cfb8 aes-ofb aes-ecb aes-ctr \
           aes-direct aes-gcm aes-gmac aes-ccm aes-keywrap aes-xts \
           aes-siv aes-eax aes-cts

all: $(EXAMPLES)

aes-cbc: aes-cbc.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-cfb: aes-cfb.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-cfb1: aes-cfb1.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-cfb8: aes-cfb8.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-ofb: aes-ofb.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-ecb: aes-ecb.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-ctr: aes-ctr.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-direct: aes-direct.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-gcm: aes-gcm.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-gmac: aes-gmac.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-ccm: aes-ccm.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-keywrap: aes-keywrap.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-xts: aes-xts.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-siv: aes-siv.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-eax: aes-eax.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

aes-cts: aes-cts.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) -I$(WOLFSSL_INSTALL_DIR)/include

.PHONY: clean

clean:
	rm -f *.o $(EXAMPLES) temp_*.bin
