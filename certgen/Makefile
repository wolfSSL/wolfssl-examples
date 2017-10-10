CC=gcc
#if you installed wolfssl to an alternate location use CFLAGS and LIBS to
#control your build:
#CFLAGS=-Wall -I/path/to/include
#LIBS=-L/path/to/lib -lwolfssl
#
#EXAMPLE:
#CFLAGS=-Wall -I/Users/khimes/work/testDir/wolf-install-dir-for-testing/include
#LIBS=-L/Users/khimes/work/testDir/wolf-install-dir-for-testing/lib -lwolfssl
#END EXAMPLE

CFLAGS=-Wall
LIBS=-lwolfssl


all:certgen_example csr_example

certgen_example:certgen_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

csr_example:csr_example.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o test.o run*
	rm newCert.*
