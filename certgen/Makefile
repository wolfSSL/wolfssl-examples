CC=gcc
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:

#EXAMPLE: set WOLF_INSTALL_DIR to point to your install location like so:

# WOLF_INSTALL_DIR=/Users/kalebhimes/work/testDir/wolf-install-dir-for-testing

#END EXAMPLE

WOLF_INSTALL_DIR=/usr/local

# ECC Examples Makefile
CC       = gcc
LIB_PATH = /usr/local
CFLAGS   = -Wall -I$(LIB_PATH)/include
LIBS     = -L$(LIB_PATH)/lib -lm

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(LIB_PATH)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
DEBUG_INC_PATHS = -MD
OPTIMIZE        = -Os

# Options
#CFLAGS+=$(DEBUG_FLAGS)
CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

all:certgen_example certgen_ca_example csr_example csr_w_ed25519_example csr_sign csr_cryptocb custom_ext custom_ext_callback

certgen_example:certgen_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

certgen_ca_example:certgen_ca_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_example:csr_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_w_ed25519_example:csr_w_ed25519_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_sign:csr_sign.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_cryptocb:csr_cryptocb.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

custom_ext:custom_ext.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

custom_ext_callback:custom_ext_callback.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o certgen_example certgen_ca_example csr_example csr_w_ed25519_example csr_sign csr_cryptocb custom_ext custom_ext_callback
	rm -f newCert.*
