CC=gcc
LIB_PATH=/usr/local
CFLAGS= -I$(LIB_PATH)/include -Wall
LIBS= -L$(LIB_PATH)/lib -lwolfssl

ech_cloudflare_test: ech_cloudflare_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o ech_cloudflare_test
