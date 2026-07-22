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

.PHONY: clean check

clean:
	rm -f *.o $(EXAMPLES) temp_*.bin

check: SHELL := /bin/bash
check: .SHELLFLAGS := -eo pipefail -c
check: aes-cbc aes-cfb aes-cfb1 aes-cfb8 aes-ofb aes-ecb aes-ctr aes-direct aes-gcm aes-gmac aes-ccm aes-keywrap aes-xts aes-siv aes-eax aes-cts
	printf 'wolfssl-examples aes-modes CI input\n' > in.txt
	./aes-cbc in.txt out-cbc.bin | grep -q 'Success!'
	./aes-cfb in.txt out-cfb.bin | grep -q 'Success!'
	./aes-cfb1 in.txt out-cfb1.bin | grep -q 'Success!'
	./aes-cfb8 in.txt out-cfb8.bin | grep -q 'Success!'
	./aes-ofb in.txt out-ofb.bin | grep -q 'Success!'
	./aes-ecb in.txt out-ecb.bin | grep -q 'Success!'
	./aes-ctr in.txt out-ctr.bin | grep -q 'Success!'
	./aes-direct in.txt out-direct.bin | grep -q 'Success!'
	./aes-gcm in.txt out-gcm.bin | grep -q 'Success!'
	./aes-gmac in.txt out-gmac.bin | grep -q 'Success!'
	./aes-ccm in.txt out-ccm.bin | grep -q 'Success!'
	./aes-keywrap in.txt out-keywrap.bin | grep -q 'Success!'
	./aes-xts in.txt out-xts.bin | grep -q 'Success!'
	./aes-siv in.txt out-siv.bin | grep -q 'Success!'
	./aes-eax in.txt out-eax.bin | grep -q 'Success!'
	./aes-cts in.txt out-cts.bin | grep -q 'Success!'
	@echo "PASS: crypto-aes-modes checks"
