/* sockets.h
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */


#ifdef USE_WINDOWS_API
    #include <winsock2.h>
    #ifdef TEST_IPV6            /* don't require newer SDK for IPV4 */
        #include <ws2tcpip.h>
        #include <wspiapi.h>
    #endif
    #define SOCKET_T SOCKET
    #define SNPRINTF _snprintf
#elif defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
    #include <string.h>
    #include "rl_net.h"
    #define SOCKET_T int
        typedef int socklen_t ;
        static unsigned long inet_addr(const char *cp)
    {
        unsigned int a[4] ; unsigned long ret ;
        sscanf(cp, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) ;
        ret = ((a[3]<<24) + (a[2]<<16) + (a[1]<<8) + a[0]) ;
        return(ret) ;
    }
        #if defined(HAVE_KEIL_RTX)
        #define sleep(t) os_dly_wait(t/1000+1) ;
    #elif defined (WOLFSSL_CMSIS_RTOS)
        #define sleep(t)  osDelay(t/1000+1) ;
    #endif
#elif defined(WOLFSSL_TIRTOS)
    #include <string.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <ti/sysbios/knl/Task.h>
    struct hostent {
        char *h_name; /* official name of host */
        char **h_aliases; /* alias list */
        int h_addrtype; /* host address type */
        int h_length; /* length of address */
        char **h_addr_list; /* list of addresses from name server */
    };
    #define SOCKET_T int
#elif defined(WOLFSSL_VXWORKS)
    #include <hostLib.h>
    #include <sockLib.h>
    #include <arpa/inet.h>
    #include <string.h>
    #include <selectLib.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <netdb.h>
    #define SOCKET_T int
#elif defined(WOLFSSL_ZEPHYR)
    #include <string.h>
    #include <sys/types.h>
    #include <net/socket.h>
    #define SOCKET_T int
    #define SOL_SOCKET 1
    #define SO_REUSEADDR 201
    #define WOLFSSL_USE_GETADDRINFO

    static unsigned long inet_addr(const char *cp)
    {
        unsigned int a[4]; unsigned long ret;
        int i, j;
        for (i=0, j=0; i<4; i++) {
            a[i] = 0;
            while (cp[j] != '.' && cp[j] != '\0') {
                a[i] *= 10;
                a[i] += cp[j] - '0';
                j++;
            }
        }
        ret = ((a[3]<<24) + (a[2]<<16) + (a[1]<<8) + a[0]) ;
        return(ret) ;
    }
#else
    #include <string.h>
    #include <sys/types.h>
#ifndef WOLFSSL_LEANPSK
    #include <unistd.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <sys/socket.h>
    #include <fcntl.h>
    #ifdef TEST_IPV6
        #include <netdb.h>
    #endif
#endif
    #define SOCKET_T int
    #ifndef SO_NOSIGPIPE
        #include <signal.h>  /* ignore SIGPIPE */
    #endif
    #define SNPRINTF snprintf
#endif /* USE_WINDOWS_API */


#ifdef WOLFSSL_KEIL_TCP_NET
    #define SOCK_LISTEN_MAX_QUEUE 1
#else
    #define SOCK_LISTEN_MAX_QUEUE 5
#endif


#ifndef WOLFSSL_SOCKET_INVALID
    #ifdef USE_WINDOWS_API
        #define WOLFSSL_SOCKET_INVALID  ((SOCKET_T)INVALID_SOCKET)
        #define WOLFSSL_SOCKET_IS_INVALID(s)  \
                                    ((SOCKET_T)(s) == WOLFSSL_SOCKET_INVALID)
    #elif defined(WOLFSSL_TIRTOS)
        #define WOLFSSL_SOCKET_INVALID  ((SOCKET_T)-1)
        #define WOLFSSL_SOCKET_IS_INVALID(s)  \
                                    ((SOCKET_T)(s) == WOLFSSL_SOCKET_INVALID)
    #else
        #define WOLFSSL_SOCKET_INVALID  (SOCKET_T)(0)
        #define WOLFSSL_SOCKET_IS_INVALID(s)  \
                                    ((SOCKET_T)(s) < WOLFSSL_SOCKET_INVALID)
    #endif
#endif /* WOLFSSL_SOCKET_INVALID */


#ifdef SAMPLE_IPV6
    typedef struct sockaddr_in6 SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET6
    static const char* const wolfSSLIP   = "::1";
#else
    typedef struct sockaddr_in  SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET
    static const char* const wolfSSLIP   = "127.0.0.1";
#endif
static const word16      wolfSSLPort = 11111;

/* HPUX doesn't use socklent_t for third parameter to accept, unless
   _XOPEN_SOURCE_EXTENDED is defined */
#if (!defined(__hpux__) && !defined(WOLFSSL_MDK_ARM) && \
        !defined(WOLFSSL_IAR_ARM) && !defined(WOLFSSL_ROWLEY_ARM) && \
        !defined(WOLFSSL_KEIL_TCP_NET)) || defined(_XOPEN_SOURCE_EXTENDED)
    typedef socklen_t* ACCEPT_THIRD_T;
#else
    typedef int*       ACCEPT_THIRD_T;
#endif


#ifdef USE_WINDOWS_API
static WC_INLINE int tcp_set_nonblocking(SOCKET_T* sockfd)
{
    int           ret = 0;
    unsigned long blocking = 1;

    ret = ioctlsocket(*sockfd, FIONBIO, &blocking);
    if (ret == SOCKET_ERROR)
        printf("ioctlsocket failed");

    return ret;
}
#elif defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET) || \
      defined(WOLFSSL_TIRTOS)|| defined(WOLFSSL_VXWORKS) || \
      defined(WOLFSSL_ZEPHYR)
static WC_INLINE int tcp_set_nonblocking(SOCKET_T* sockfd)
{
    (void)sockfd;
    return 0;
}
#else
static WC_INLINE int tcp_set_nonblocking(SOCKET_T* sockfd)
{
    int ret = 0;

    int flags = fcntl(*sockfd, F_GETFL, 0);
    if (flags < 0) {
        printf("fcntl get failed");
        ret = -1;
    }
    flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) {
        printf("fcntl set failed");
        ret = -1;
    }

    return ret;
}
#endif


#if defined(HAVE_GETADDRINFO) || defined(WOLF_C99)

#ifndef SAMPLE_IPV6

static WC_INLINE int get_addr(SOCKADDR_IN_T* addr, const char* peer,
                              word16 port, int udp, int sctp)
{
    int              ret = 0;
    struct addrinfo  hints;
    struct addrinfo* answer = NULL;
    char             strPort[6];

    (void)sctp;

    XMEMSET(&hints, 0, sizeof(hints));

    hints.ai_family   = AF_INET_V;
    if (udp) {
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
    }
#ifdef WOLFSSL_SCTP
    else if (sctp) {
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_SCTP;
    }
#endif
    else {
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
    }

    SNPRINTF(strPort, sizeof(strPort), "%d", port);
    strPort[sizeof(strPort)-1] = '\0';

    ret = getaddrinfo(peer, strPort, &hints, &answer);
    if (ret < 0 || answer == NULL) {
        printf("getaddrinfo failed\n");
        ret = -1;
    }
    else
        XMEMCPY(addr, answer->ai_addr, answer->ai_addrlen);
    freeaddrinfo(answer);

    return ret;
}

#else /* SAMPLE_IPV6 */

static WC_INLINE int get_addr(SOCKADDR_IN_T* addr, const char* peer,
                              word16 port, int udp, int sctp)
{
    int ret = 0;
    struct zsock_addrinfo hints, *addrInfo;
    char portStr[6];

    (void)sctp;

    XSNPRINTF(portStr, sizeof(portStr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
    ret = getaddrinfo((char*)peer, portStr, &hints, &addrInfo);
    if (ret == 0)
        XMEMCPY(addr, addrInfo->ai_addr, sizeof(*addr));

    return ret;
}

#endif /* SAMPLE_IPV6 */

#else /* !GETADDRINFO */

#ifndef SAMPLE_IPV6

static WC_INLINE int get_addr(SOCKADDR_IN_T* addr, const char* peer,
                              word16 port, int udp, int sctp)
{
    int ret = 0;
    #if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
        int err;
        struct hostent* entry = gethostbyname(peer, &err);
    #elif defined(WOLFSSL_TIRTOS)
        struct hostent* entry = DNSGetHostByName(peer);
    #elif defined(WOLFSSL_VXWORKS)
        struct hostent* entry = (struct hostent*)hostGetByName(
                                                           (char*)peer);
    #else
        struct hostent* entry = gethostbyname(peer);
    #endif

    (void)port;
    (void)udp;
    (void)sctp;

    if (entry)
        XMEMCPY(&addr->sin_addr.s_addr, entry->h_addr_list[0], entry->h_length);
    else
        ret = -1;

    return ret;
}

#else

static WC_INLINE int get_addr(SOCKADDR_IN_T* addr, const char* peer,
                              word16 port, int udp, int sctp)
{
    (void)peer;
    (void)port;
    (void)udp;
    (void)sctp;

    printf("no ipv6 getaddrinfo, loopback only\n");
    addr->sin6_addr = in6addr_loopback;

    return 0;
}

#endif /* SAMPLE_IPV6 */

#endif /* !GETADDRINFO */



static WC_INLINE int build_addr(SOCKADDR_IN_T* addr, const char* peer,
                                word16 port, int udp, int sctp)
{
    int ret = 0;
    int useLookup = 0;
    (void)useLookup;

    if (addr == NULL) {
        printf("invalid argument to build_addr, addr is NULL\n");
        ret = -1;
    }

    if (ret == 0) {
        XMEMSET(addr, 0, sizeof(SOCKADDR_IN_T));

#ifndef SAMPLE_IPV6
    #if defined(WOLFSSL_MDK_ARM) || defined(WOLFSSL_KEIL_TCP_NET)
        addr->sin_family = PF_INET;
    #else
        addr->sin_family = AF_INET_V;
    #endif
        addr->sin_port = XHTONS(port);
        if ((size_t)peer == INADDR_ANY)
            addr->sin_addr.s_addr = INADDR_ANY;
        else {
            /* peer could be in human readable form */
            if (isalpha((int)peer[0])) {
                ret = get_addr(addr, peer, port, udp, sctp);
                if (ret == 0)
                    useLookup = 1;
                else
                    printf("no entry for host");
            }
            if (ret == 0 && !useLookup)
                addr->sin_addr.s_addr = inet_addr(peer);
        }
#else /* SAMPLE_IPV6 */
        addr->sin6_family = AF_INET_V;
        addr->sin6_port = XHTONS(port);
        if ((size_t)peer == INADDR_ANY)
            addr->sin6_addr = in6addr_any;
        else
            ret = get_addr(addr, peer, port, udp, sctp);
#endif /* SAMPL_IPV6 */
    }

    return ret;
}


#ifdef SO_NOSIGPIPE
static WC_INLINE int tcp_set_socket_no_sig_pipe(SOCKET_T sockfd)
{
    int       ret;
    int       on = 1;
    socklen_t len = sizeof(on);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &on, len);
    if (ret < 0)
        printf("setsockopt SO_NOSIGPIPE failed\n");

    return ret;
}
#elif defined(WOLFSSL_MDK_ARM) || defined (WOLFSSL_TIRTOS) || \
      defined(WOLFSSL_KEIL_TCP_NET) || defined(WOLFSSL_ZEPHYR) || \
      defined(USE_WINDOWS_API)
static WC_INLINE int tcp_set_socket_no_sig_pipe(SOCKET_T sockfd)
{
    (void)sockfd;
    return 0;
}
#else
static WC_INLINE int tcp_set_socket_no_sig_pipe(SOCKET_T sockfd)
{
    signal(SIGPIPE, SIG_IGN);
    return 0;
}
#endif

#ifdef TCP_NODELAY
static WC_INLINE int tcp_set_socket_tcp_no_delay(SOCKET_T sockfd)
{
    int       ret;
    int       on = 1;
    socklen_t len = sizeof(on);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, len);
    if (ret < 0)
        printf("setsockopt TCP_NODELAY failed\n");

    return ret;
}
#else
static WC_INLINE int tcp_set_socket_tcp_no_delay(SOCKET_T sockfd)
{
    return 0;
}
#endif

static WC_INLINE int tcp_set_socket_opts(SOCKET_T sockfd, int udp, int sctp)
{
    int ret = 0;

    ret = tcp_set_socket_no_sig_pipe(sockfd);
    if (ret == 0 && !udp && !sctp)
        ret = tcp_set_socket_tcp_no_delay(sockfd);

    return ret;
}

static WC_INLINE int tcp_socket(SOCKET_T* sockfd, int udp, int sctp)
{
    int ret = 0;

    if (udp)
        *sockfd = socket(AF_INET_V, SOCK_DGRAM, IPPROTO_UDP);
#ifdef WOLFSSL_SCTP
    else if (sctp)
        *sockfd = socket(AF_INET_V, SOCK_STREAM, IPPROTO_SCTP);
#endif
    else
        *sockfd = socket(AF_INET_V, SOCK_STREAM, IPPROTO_TCP);

    if(WOLFSSL_SOCKET_IS_INVALID(*sockfd)) {
        printf("socket failed\n");
        ret = -1;
    }

    if (ret == 0)
        ret = tcp_set_socket_opts(*sockfd, udp, sctp);

   return ret;
}

static WC_INLINE int tcp_connect(SOCKET_T* sockfd, const char* ip, word16 port,
                                 int udp, int sctp, WOLFSSL* ssl)
{
    int           ret = 0;
    SOCKADDR_IN_T addr;

    ret = build_addr(&addr, ip, port, udp, sctp);
    if (ret != 0)
        printf("build address failed\n");
    if (ret == 0) {
        if (udp)
            wolfSSL_dtls_set_peer(ssl, &addr, sizeof(addr));
        ret = tcp_socket(sockfd, udp, sctp);
    }

    if (ret == 0 && !udp) {
        if (connect(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) !=
                                                                            0) {
            printf("tcp connect failed");
            ret = -1;
        }
    }

    return ret;
}


#if defined(USE_WINDOWS_API) || defined(WOLFSSL_MDK_ARM) || \
                        defined(WOLFSSL_KEIL_TCP_NET) || defined(WOLFSSL_ZEPHYR)
static WC_INLINE int tcp_set_socket_reuseaddr(SOCKET_T sockfd)
{
    (void)sockfd;

    return 0;
}
#else
static WC_INLINE int tcp_set_socket_reuseaddr(SOCKET_T sockfd)
{
    int       ret;
    int       on  = 1;
    socklen_t len = sizeof(on);

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (ret < 0)
        printf("setsockopt SO_REUSEADDR failed\n");

    return ret;
}
#endif

#if defined(USE_WINDOWS_API) || defined(WOLFSSL_TIRTOS) || \
                                                         defined(WOLFSSL_ZEPHYR)
static WC_INLINE int tcp_get_sock_name(SOCKET_T sockfd, SOCKADDR_IN_T* addr,
                                       word16* port)
{
    (void)sockfd;
    (void)addr;
    (void)port;

    return 0;
}
#else
static WC_INLINE int tcp_get_sock_name(SOCKET_T sockfd, SOCKADDR_IN_T* addr,
                                       word16* port)
{
    socklen_t len = sizeof(*addr);

    if (getsockname(sockfd, (struct sockaddr*)addr, &len) == 0) {
    #ifndef SAMPLE_IPV6
        *port = XNTOHS(addr->sin_port);
    #else
        *port = XNTOHS(addr->sin6_port);
    #endif
    }

    return 0;
}
#endif

static WC_INLINE int tcp_listen(SOCKET_T* sockfd, word16* port, int useAnyAddr,
                                int udp, int sctp)
{
    int           ret;
    SOCKADDR_IN_T addr;

    /* don't use INADDR_ANY by default, firewall may block, make user switch
       on */
    ret = build_addr(&addr, (useAnyAddr ? INADDR_ANY : wolfSSLIP), *port, udp,
                                                                          sctp);
    if (ret != 0)
        printf("build address failed\n");
    if (ret == 0) {
        ret = tcp_socket(sockfd, udp, sctp);
        if (ret != 0)
            printf("socket listen failed\n");
    }

    if (ret == 0)
        ret = tcp_set_socket_reuseaddr(*sockfd);

    if (ret == 0) {
        if (bind(*sockfd, (const struct sockaddr*)&addr, sizeof(addr)) != 0) {
            printf("tcp bind failed\n");
            ret = -1;
        }
    }
    if (ret == 0 && !udp) {
        if (listen(*sockfd, SOCK_LISTEN_MAX_QUEUE) != 0) {
            printf("tcp listen failed\n");
            ret = -1;
        }
    }
    if (ret == 0 && *port == 0)
        ret = tcp_get_sock_name(*sockfd, &addr, port);

   return ret;
}

static WC_INLINE int tcp_accept(SOCKET_T* sockfd, SOCKET_T* clientfd)
{
    int           ret = 0;
    SOCKADDR_IN_T client;
    socklen_t     client_len = sizeof(client);

    *clientfd = accept(*sockfd, (struct sockaddr*)&client,
                      (ACCEPT_THIRD_T)&client_len);
    if (WOLFSSL_SOCKET_IS_INVALID(*clientfd)) {
       printf("tcp accept failed\n");
       ret = -1;
    }

    return ret;
}


