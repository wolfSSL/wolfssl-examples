CC=gcc
LIB_PATH=/usr/local
CFLAGS= -I$(LIB_PATH)/include -Wall
LIBS= -L$(LIB_PATH)/lib -lwolfssl

hpke_test: hpke_test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o hpke_test
