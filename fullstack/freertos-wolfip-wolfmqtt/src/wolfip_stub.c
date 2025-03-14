/* wolfip_stub.c - Stub implementation for wolfIP */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "wolfip_compat.h"

/* Stub implementation of wolfIP functions */
struct wolfIP *wolfIP_init(void) {
    printf("wolfIP_init called\n");
    return (struct wolfIP *)calloc(1, sizeof(struct wolfIP));
}

void wolfIP_free(struct wolfIP *s) {
    printf("wolfIP_free called\n");
    if (s) free(s);
}

int wolfIP_register_ll(struct wolfIP *s, int fd, void *mac, void *rx_cb, void *tx_cb) {
    printf("wolfIP_register_ll called\n");
    return 0;
}

int wolfIP_ipconfig_set(struct wolfIP *s, struct wolfIP_ipconfig *cfg) {
    printf("wolfIP_ipconfig_set called\n");
    return 0;
}

void wolfIP_poll(struct wolfIP *s, uint64_t now) {
    /* No-op */
    (void)s;
    (void)now;
}

int wolfIP_sock_socket(struct wolfIP *s, int domain, int type, int protocol) {
    return socket(domain, type, protocol);
}

int wolfIP_sock_bind(struct wolfIP *s, int sockfd, const struct wolfIP_sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, (const struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_listen(struct wolfIP *s, int sockfd, int backlog) {
    return listen(sockfd, backlog);
}

int wolfIP_sock_accept(struct wolfIP *s, int sockfd, struct wolfIP_sockaddr *addr, socklen_t *addrlen) {
    return accept(sockfd, (struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_connect(struct wolfIP *s, int sockfd, const struct wolfIP_sockaddr *addr, socklen_t addrlen) {
    return connect(sockfd, (const struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_send(struct wolfIP *s, int sockfd, const void *buf, size_t len, int flags) {
    return send(sockfd, buf, len, flags);
}

int wolfIP_sock_sendto(struct wolfIP *s, int sockfd, const void *buf, size_t len, int flags,
                     const struct wolfIP_sockaddr *dest_addr, socklen_t addrlen) {
    return sendto(sockfd, buf, len, flags, (const struct sockaddr *)dest_addr, addrlen);
}

int wolfIP_sock_recv(struct wolfIP *s, int sockfd, void *buf, size_t len, int flags) {
    return recv(sockfd, buf, len, flags);
}

int wolfIP_sock_recvfrom(struct wolfIP *s, int sockfd, void *buf, size_t len, int flags,
                       struct wolfIP_sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, (struct sockaddr *)src_addr, addrlen);
}

int wolfIP_sock_close(struct wolfIP *s, int sockfd) {
    return close(sockfd);
}

uint16_t wolfIP_htons(uint16_t hostshort) {
    return htons(hostshort);
}

uint32_t wolfIP_inet_addr(const char *cp) {
    struct in_addr addr;
    inet_pton(AF_INET, cp, &addr);
    return addr.s_addr;
}
