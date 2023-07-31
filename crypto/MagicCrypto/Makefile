bin_files = client server
all: $(bin_files)

DEPS=common.h

CC=gcc
#CC=clang -fsanitize=address

DEBUGOPT=
#DEBUGOPT=-DWOLFSSL_DEBUG_TLS -DDEBUG_WOLFSSL -DDEBUG_CRYPTOCB
#DEBUGOPT=-DWOLFSSL_DEBUG_TLS -DDEBUG_WOLFSSL

COMMONOPT=-O0 -g -I.. -Iinclude -Llib -lwolfssl -lMagicCrypto -lm -DWOLF_CRYPTO_CB 
#COMMONOPT=-O0 -g -I../ -Iinclude -lwolfssl -Llib -lMagicCrypto -lm

%: %.c $(DEPS)
	$(CC) $< $(DEBUGOPT) $(COMMONOPT) -o $@

clean:
	rm -f $(bin_files)
