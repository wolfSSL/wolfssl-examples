CC=gcc
WOLF_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLF_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLF_INSTALL_DIR)/lib -lwolfssl -lwolftpm

all:evp_tpm

evp_tpm:evp_tpm.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o evp_tpm
