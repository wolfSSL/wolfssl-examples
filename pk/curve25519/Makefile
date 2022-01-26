CC=gcc
LIB_PATH=/usr/local
CFLAGS= -I$(LIB_PATH)/include -Wall
LIBS= -L$(LIB_PATH)/lib -lwolfssl

curve25519_test: curve25519_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o curve25519_test
