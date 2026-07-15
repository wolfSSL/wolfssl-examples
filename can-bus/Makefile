CC=gcc
WOLFSSL_INSTALL_DIR=/usr/local
LIBS=-L$(WOLFSSL_INSTALL_DIR)/lib -lwolfssl
CFLAGS=-g -Wno-cpp -Wall -Wextra -Wpedantic -Wdeclaration-after-statement -I$(WOLFSSL_INSTALL_DIR)/include

COMMON_OBJS=common.o
CLIENT_OBJS=client.o
SERVER_OBJS=server.o

all: client server

%.o: %.c
	@$(CC) -c $< -o $@ $(CFLAGS)

client: $(COMMON_OBJS) $(CLIENT_OBJS)
	@$(CC) -o $@ $(COMMON_OBJS) $(CLIENT_OBJS) $(CFLAGS) $(LIBS)

server: $(COMMON_OBJS) $(SERVER_OBJS)
	@$(CC) -o $@ $(COMMON_OBJS) $(SERVER_OBJS) $(CFLAGS) $(LIBS)

clean:
	@rm -f *.o
	@rm -f client
	@rm -f server
