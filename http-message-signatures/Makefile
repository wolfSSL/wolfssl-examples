CC ?= gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS = -I$(WOLFSSL_INSTALL_DIR)/include -I. -Wall -Wextra
LDFLAGS = -L$(WOLFSSL_INSTALL_DIR)/lib
LDLIBS = -lwolfssl

COMMON_SRC = common/wc_sf.c common/wc_http_sig.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

TARGETS = sign_request test_vectors http_server_verify http_client_signed

all: $(TARGETS)

sign_request: sign_request.o $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

test_vectors: test_vectors.o $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

http_server_verify: http_server_verify.o $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

http_client_signed: http_client_signed.o $(COMMON_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all clean test

clean:
	rm -f $(TARGETS) *.o common/*.o

test: test_vectors
	./test_vectors
