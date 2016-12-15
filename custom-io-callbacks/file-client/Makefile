CC=clang
# if you installed wolfssl to /usr/local please make sure the path to
# /usr/local/include is in your -I path and /usr/local/lib is in your
# LD_LIBRARY_PATH
CFLAGS=-Wall
LIBS= -L/usr/local/lib -lwolfssl

all:start-client

start-client:file-client.o
	$(CC) -o $@ $(LIBS) $^ $(CFLAGS)

.PHONY: clean all

clean:
	rm -f *.o file-client.o start-client
