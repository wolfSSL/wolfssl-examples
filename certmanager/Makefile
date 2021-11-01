CC=gcc
CFLAGS=-Wall
LIBS= -lwolfssl

all: certloadverifybuffer certverify

certloadverifybuffer: certloadverifybuffer.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
certverify: certverify.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o certverify certloadverifybuffer
