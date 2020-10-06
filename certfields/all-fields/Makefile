CC=gcc
WOLF_INSTALL_DIR=/usr/local

CFLAGS=-I$(WOLF_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLF_INSTALL_DIR)/lib -lwolfssl

app: main.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o app
