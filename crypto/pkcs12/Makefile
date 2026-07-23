CC=gcc
CFLAGS=-Wall
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all:pkcs12-example pkcs12-create-example

pkcs12-example: pkcs12-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pkcs12-create-example: pkcs12-create-example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean check

clean:
	rm -f *.o pkcs12-example pkcs12-create-example output.p12 example.log create.log

# Redirect then grep (no pipe) so the binary's nonzero exit fails the recipe and
# there is no SIGPIPE on the long DER dump; both examples now return nonzero on
# failure, so the exit code carries the result and the grep confirms the output.
check: pkcs12-example pkcs12-create-example
	./pkcs12-example > example.log
	grep -q 'return value of parsing pkcs12 = 0 SUCCESS' example.log
	./pkcs12-create-example > create.log
	grep -q 'Printing PKCS12 DER file to output.p12' create.log
	@echo "PASS: crypto-pkcs12 checks"
