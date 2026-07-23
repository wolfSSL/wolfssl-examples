CC=gcc -g
WOLFSSL_INSTALL_DIR=/usr/local
CFLAGS=-Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS= -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: certvfy certsigvfy sigvfycert

certvfy: certvfy.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
certsigvfy: certsigvfy.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
sigvfycert: sigvfycert.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o certvfy certsigvfy sigvfycert

check: certvfy certsigvfy sigvfycert
	out=$$(./certvfy) && printf '%s' "$$out" | grep -q 'Verification Successful!'
	out=$$(./certsigvfy) && printf '%s' "$$out" | grep -q 'Verification Successful!'
	out=$$(./sigvfycert) && printf '%s' "$$out" | grep -q 'Verification Successful!'
	@echo "PASS: certvfy checks"
