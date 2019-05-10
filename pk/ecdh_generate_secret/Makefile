CC=gcc
LIB_PATH=/usr/local
CFLAGS= -I$(LIB_PATH)/include -Wall
LIBS= -L$(LIB_PATH)/lib -lwolfssl

ecdh_gen_secret: ecdh_gen_secret.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.der *.x963 *.o ecdh_gen_secret
