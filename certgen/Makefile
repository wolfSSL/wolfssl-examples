CC=gcc
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:

#EXAMPLE: set WOLF_INSTALL_DIR to point to your install location like so:

# WOLF_INSTALL_DIR=/Users/kalebhimes/work/testDir/wolf-install-dir-for-testing

#END EXAMPLE

WOLF_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLF_INSTALL_DIR)/include -Wall
LIBS=-L$(WOLF_INSTALL_DIR)/lib -lwolfssl


all:certgen_example csr_example csr_w_ed25519_example csr_sign csr_cryptocb custom_ext_callback`

certgen_example:certgen_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_example:csr_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_w_ed25519_example:csr_w_ed25519_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_sign:csr_sign.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_cryptocb:csr_cryptocb.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

custom_ext_callback:custom_ext_callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o certgen_example csr_example csr_w_ed25519_example csr_sign csr_cryptocb custom_ext_callback
	rm -f newCert.*
