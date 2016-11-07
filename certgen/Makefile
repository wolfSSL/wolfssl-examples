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


all:run_certgen_example

run_certgen_example:test.o
	$(CC) -o $@ $^ $(CFLAGS) $(CPPFLAGS) $(LIBS)

.PHONY: clean all

clean:
	rm -f *.o test.o run*
