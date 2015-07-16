CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

certverify: certverify.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o certverify
