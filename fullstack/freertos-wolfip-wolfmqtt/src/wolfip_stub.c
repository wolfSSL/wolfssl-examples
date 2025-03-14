/* wolfip_stub.c - Stub implementation for wolfIP */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../../../../../../wolfip/wolfip.h"

/* Define a simple wolfIP structure for our stub implementation */
struct wolfIP {
    int initialized;
    ip4 ip;
    ip4 mask;
    ip4 gw;
};

/* Static instance for our stub */
static struct wolfIP wolfip_instance;

/* Stub implementation of wolfIP functions */
void wolfIP_init(struct wolfIP *s) {
    printf("wolfIP_init called\n");
    if (s) {
        memset(s, 0, sizeof(struct wolfIP));
        s->initialized = 1;
    }
}

void wolfIP_init_static(struct wolfIP **s) {
    printf("wolfIP_init_static called\n");
    if (s) {
        *s = &wolfip_instance;
        wolfIP_init(*s);
    }
}

int wolfIP_poll(struct wolfIP *s, uint64_t now) {
    /* No-op */
    (void)s;
    (void)now;
    return 0;
}

void wolfIP_recv(struct wolfIP *s, void *buf, uint32_t len) {
    /* No-op */
    (void)s;
    (void)buf;
    (void)len;
}

void wolfIP_ipconfig_set(struct wolfIP *s, ip4 ip, ip4 mask, ip4 gw) {
    printf("wolfIP_ipconfig_set called with IP: %08x, Mask: %08x, GW: %08x\n", 
           ip, mask, gw);
    if (s) {
        s->ip = ip;
        s->mask = mask;
        s->gw = gw;
    }
}

void wolfIP_ipconfig_get(struct wolfIP *s, ip4 *ip, ip4 *mask, ip4 *gw) {
    if (s && ip && mask && gw) {
        *ip = s->ip;
        *mask = s->mask;
        *gw = s->gw;
    }
}

struct ll *wolfIP_getdev(struct wolfIP *s) {
    (void)s;
    return NULL;
}

void wolfIP_register_callback(struct wolfIP *s, int sock_fd, void (*cb)(int sock_fd, uint16_t events, void *arg), void *arg) {
    (void)s;
    (void)sock_fd;
    (void)cb;
    (void)arg;
}

int wolfIP_sock_socket(struct wolfIP *s, int domain, int type, int protocol) {
    (void)s;
    return socket(domain, type, protocol);
}

int wolfIP_sock_bind(struct wolfIP *s, int sockfd, const struct wolfIP_sockaddr *addr, socklen_t addrlen) {
    (void)s;
    return bind(sockfd, (const struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_listen(struct wolfIP *s, int sockfd, int backlog) {
    (void)s;
    return listen(sockfd, backlog);
}

int wolfIP_sock_accept(struct wolfIP *s, int sockfd, struct wolfIP_sockaddr *addr, socklen_t *addrlen) {
    (void)s;
    return accept(sockfd, (struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_connect(struct wolfIP *s, int sockfd, const struct wolfIP_sockaddr *addr, socklen_t addrlen) {
    (void)s;
    return connect(sockfd, (const struct sockaddr *)addr, addrlen);
}

int wolfIP_sock_send(struct wolfIP *s, int sockfd, const void *buf, size_t len, int flags) {
    (void)s;
    return send(sockfd, buf, len, flags);
}

int wolfIP_sock_sendto(struct wolfIP *s, int sockfd, const void *buf, size_t len, int flags,
                     const struct wolfIP_sockaddr *dest_addr, socklen_t addrlen) {
    (void)s;
    return sendto(sockfd, buf, len, flags, (const struct sockaddr *)dest_addr, addrlen);
}

int wolfIP_sock_recv(struct wolfIP *s, int sockfd, void *buf, size_t len, int flags) {
    (void)s;
    return recv(sockfd, buf, len, flags);
}

int wolfIP_sock_recvfrom(struct wolfIP *s, int sockfd, void *buf, size_t len, int flags,
                       struct wolfIP_sockaddr *src_addr, socklen_t *addrlen) {
    (void)s;
    return recvfrom(sockfd, buf, len, flags, (struct sockaddr *)src_addr, addrlen);
}

int wolfIP_sock_close(struct wolfIP *s, int sockfd) {
    (void)s;
    return close(sockfd);
}

/* Additional socket functions */
int wolfIP_sock_write(struct wolfIP *s, int sockfd, const void *buf, size_t len) {
    (void)s;
    return write(sockfd, buf, len);
}

int wolfIP_sock_read(struct wolfIP *s, int sockfd, void *buf, size_t len) {
    (void)s;
    return read(sockfd, buf, len);
}

int wolfIP_sock_getpeername(struct wolfIP *s, int sockfd, struct wolfIP_sockaddr *addr, const socklen_t *addrlen) {
    (void)s;
    return getpeername(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

int wolfIP_sock_getsockname(struct wolfIP *s, int sockfd, struct wolfIP_sockaddr *addr, const socklen_t *addrlen) {
    (void)s;
    return getsockname(sockfd, (struct sockaddr *)addr, (socklen_t *)addrlen);
}

/* DNS client stub */
int nslookup(struct wolfIP *s, const char *name, uint16_t *id, void (*lookup_cb)(uint32_t ip)) {
    (void)s;
    (void)name;
    (void)id;
    (void)lookup_cb;
    return -1;
}

/* DHCP client stub */
int dhcp_client_init(struct wolfIP *s) {
    (void)s;
    return -1;
}

int dhcp_bound(struct wolfIP *s) {
    (void)s;
    return 0;
}

/* Utility functions */
uint16_t wolfIP_htons(uint16_t hostshort) {
    return htons(hostshort);
}

uint32_t wolfIP_inet_addr(const char *cp) {
    struct in_addr addr;
    inet_pton(AF_INET, cp, &addr);
    return addr.s_addr;
}
