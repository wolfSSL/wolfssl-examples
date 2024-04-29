CC=gcc -g
CFLAGS=-Wall
LIBS= -lwolfssl

all: certvfy certsigvfy sigvfycert

certvfy: certvfy.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
certsigvfy: certsigvfy.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
sigvfycert: sigvfycert.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o certvfy certsigvfy sigvfycert
