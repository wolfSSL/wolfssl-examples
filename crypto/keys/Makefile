CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

ecc_keys: ecc_keys.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.der *.x963 *.o ecc_keys
