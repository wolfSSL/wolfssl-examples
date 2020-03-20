CC=gcc
WOLFPATH=/usr/local/lib
CFLAGS=-Wall -I$(WOLFPATH)/include -g
#LIBS= -lwolfssl -lm
LIBS= -L$(WOLFPATH)/lib -lwolfssl -lm

all: srp srp_gen

srp.o: srp.c srp_params.h srp_store.h
	$(CC) -c -o $@ srp.c $(CFLAGS)

srp_gen.o: srp_gen.c srp_params.h
	$(CC) -c -o $@ srp_gen.c $(CFLAGS)

srp: srp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

srp_gen: srp_gen.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.der *.x963 *.o srp srp_gen
