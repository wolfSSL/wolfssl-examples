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

check: aes-cbc aes-cfb aes-cfb1 aes-cfb8 aes-ofb aes-ecb aes-ctr aes-direct aes-gcm aes-gmac aes-ccm aes-keywrap aes-xts aes-siv aes-eax aes-cts
	printf 'wolfssl-examples aes-modes CI input\n' > in.txt
	out=$$(./aes-cbc in.txt out-cbc.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-cfb in.txt out-cfb.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-cfb1 in.txt out-cfb1.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-cfb8 in.txt out-cfb8.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-ofb in.txt out-ofb.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-ecb in.txt out-ecb.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-ctr in.txt out-ctr.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-direct in.txt out-direct.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-gcm in.txt out-gcm.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-gmac in.txt out-gmac.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-ccm in.txt out-ccm.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-keywrap in.txt out-keywrap.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-xts in.txt out-xts.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-siv in.txt out-siv.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-eax in.txt out-eax.bin) && printf '%s' "$$out" | grep -q 'Success!'
	out=$$(./aes-cts in.txt out-cts.bin) && printf '%s' "$$out" | grep -q 'Success!'
	@echo "PASS: crypto-aes-modes checks"
