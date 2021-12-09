CC=gcc
LIBS=-lwolfssl
CFLAGS=-Iisotp-c -Wno-cpp -Wall -Wextra -Wdeclaration-after-statement

COMMON_OBJS=isotp-c/isotp.o common.o
CLIENT_OBJS=client.o
SERVER_OBJS=server.o

all: client server

isotp-c/isotp.o: isotp-c/isotp.c
	@$(CC) -c $< -o $@

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
	@rm -f isotp-c/isotp.o
