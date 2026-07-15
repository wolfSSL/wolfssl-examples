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

.PHONY: clean

clean:
	rm -f *.o certvfy certsigvfy sigvfycert
