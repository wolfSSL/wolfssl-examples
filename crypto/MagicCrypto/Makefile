bin_files = client server
WOLFSSL_INSTALL_DIR=/usr/local
all: $(bin_files)

DEPS=common.h

CC=gcc
#CC=clang -fsanitize=address

DEBUGOPT=
#DEBUGOPT=-DWOLFSSL_DEBUG_TLS -DDEBUG_WOLFSSL -DDEBUG_CRYPTOCB
#DEBUGOPT=-DWOLFSSL_DEBUG_TLS -DDEBUG_WOLFSSL

COMMONOPT=-O0 -g -IMagicCrypto/include -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl -lm -DWOLF_CRYPTO_CB 

%: %.c $(DEPS)
	$(CC) $< $(DEBUGOPT) $(COMMONOPT) -o $@

clean:
	rm -f $(bin_files)
