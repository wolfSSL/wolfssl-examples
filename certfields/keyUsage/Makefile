CC=gcc
WOLFSSL_DIR=/usr/local
CFLAGS=-Wall -I$(WOLFSSL_DIR)/include
LIBS= -L$(WOLFSSL_DIR)/lib -lwolfssl

run: test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o run
