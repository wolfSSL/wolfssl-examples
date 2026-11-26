# Set to the --prefix wolfSSL was configured and installed with
WOLFSSL_INSTALL_DIR = /usr/local

CC     = gcc
CFLAGS = -Wall -I$(WOLFSSL_INSTALL_DIR)/include
LIBS   = -L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl

all: wolfentropy_example

wolfentropy_example: wolfentropy_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o wolfentropy_example
