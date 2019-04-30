CC=gcc

#WOLFPATH=/usr/local
WOLFPATH=/Users/kalebhimes/work/testDir/wolf-install-dir-for-testing
CFLAGS= -I$(WOLFPATH)/include -Wall
LIBS= -L$(WOLFPATH)/lib -lwolfssl

app: main.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o app
