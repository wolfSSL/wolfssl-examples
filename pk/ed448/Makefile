CC=gcc
WOLF_PATH=/usr/local
CFLAGS=-I$(WOLF_PATH)/include -Wall
LIBS=-L$(WOLF_PATH)/lib -lwolfssl -lm

SRC=$(wildcard *.c)
TARGETS=$(patsubst %.c, %, $(SRC))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGETS) *.der
