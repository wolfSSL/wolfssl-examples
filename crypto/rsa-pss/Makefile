CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

rsa-pss: rsa-pss.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) 

.PHONY: clean

clean:
	rm -f *.o rsa-pss
