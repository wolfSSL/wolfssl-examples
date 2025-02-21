#include "wolfip_freertos.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <errno.h>

/* Implementation of wolfIP's required random number generator */
uint32_t wolfIP_getrandom(void) {
    uint32_t ret;
    getrandom(&ret, sizeof(ret), 0);
    return ret;
}

static struct wolfIP *g_wolfip = NULL;
static TaskHandle_t g_network_task = NULL;
static int tap_fd = -1;

/* TUN/TAP device functions */
static int tap_init(struct ll *dev, const char *ifname) {
    struct ifreq ifr;
    int sock_fd;

    if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Error opening /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    if (ioctl(tap_fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl TUNSETIFF");
        close(tap_fd);
        return -1;
    }

    /* Get MAC address */
    if (ioctl(tap_fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFHWADDR");
        close(tap_fd);
        return -1;
    }

    strncpy(dev->ifname, ifname, sizeof(dev->ifname) - 1);
    memcpy(dev->mac, ifr.ifr_hwaddr.sa_data, 6);
    dev->mac[5] ^= 1; /* Make MAC unique */

    /* Configure network interface */
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        close(tap_fd);
        return -1;
    }

    /* Set interface UP */
    if (ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCGIFFLAGS");
        close(sock_fd);
        return -1;
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(sock_fd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sock_fd);
        return -1;
    }

    /* Configure IP address */
    struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("192.168.1.10");
    if (ioctl(sock_fd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl SIOCSIFADDR");
        close(sock_fd);
        return -1;
    }

    /* Configure netmask */
    addr->sin_addr.s_addr = inet_addr("255.255.255.0");
    if (ioctl(sock_fd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("ioctl SIOCSIFNETMASK");
        close(sock_fd);
        return -1;
    }

    close(sock_fd);
    return 0;
}

static int tap_poll(struct ll *ll, void *buf, uint32_t len) {
    struct pollfd pfd;
    int ret;

    pfd.fd = tap_fd;
    pfd.events = POLLIN;

    do {
        ret = poll(&pfd, 1, 1); /* Short timeout */
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        perror("poll");
        return -1;
    }
    if (ret == 0) {
        return 0;
    }

    do {
        ret = read(tap_fd, buf, len);
    } while (ret < 0 && errno == EINTR);

    return ret;
}

static int tap_send(struct ll *ll, void *buf, uint32_t len) {
    return write(tap_fd, buf, len);
}

/* Network task implementation */
static void wolfIP_NetworkTask(void *pvParameters) {
    TickType_t last_wake_time;
    const TickType_t frequency = pdMS_TO_TICKS(WOLFIP_POLL_INTERVAL_MS);
    struct timeval tv;
    
    last_wake_time = xTaskGetTickCount();
    
    while (1) {
        gettimeofday(&tv, NULL);
        wolfIP_poll(g_wolfip, tv.tv_sec * 1000 + tv.tv_usec / 1000);
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

int wolfIP_FreeRTOS_Init(void) {
    struct ll *tapdev;
    
    /* Initialize wolfIP */
    wolfIP_init_static(&g_wolfip);
    if (!g_wolfip) {
        printf("Failed to initialize wolfIP\n");
        return -1;
    }
    
    /* Setup TUN/TAP interface */
    tapdev = wolfIP_getdev(g_wolfip);
    if (!tapdev) {
        printf("Failed to get device from wolfIP\n");
        return -1;
    }

    /* Initialize TAP device */
    if (tap_init(tapdev, "wtap0") < 0) {
        printf("Failed to initialize TAP device\n");
        return -1;
    }

    /* Set device callbacks */
    tapdev->poll = tap_poll;
    tapdev->send = tap_send;
    
    /* Configure IP settings */
    wolfIP_ipconfig_set(g_wolfip, 
        atoip4("192.168.1.10"),     /* IP */
        atoip4("255.255.255.0"),    /* Netmask */
        atoip4("192.168.1.1"));     /* Gateway */
        
    return 0;
}

static void UDP_Echo_Task(void* pvParameters) {
    int sockfd;
    uint8_t buf[1024];
    int ret;
    struct wolfIP_sockaddr_in addr;
    struct wolfIP_sockaddr_in client_addr;
    socklen_t client_len;

    sockfd = wolfIP_sock_socket(g_wolfip, AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Failed to create UDP socket\n");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_TEST_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (wolfIP_sock_bind(g_wolfip, sockfd, (struct wolfIP_sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Failed to bind UDP socket\n");
        wolfIP_sock_close(g_wolfip, sockfd);
        return;
    }

    printf("UDP Echo Server running on port %d\n", UDP_TEST_PORT);

    while (1) {
        client_len = sizeof(client_addr);
        ret = wolfIP_sock_recvfrom(g_wolfip, sockfd, buf, sizeof(buf), 0,
                                (struct wolfIP_sockaddr*)&client_addr, &client_len);
        if (ret > 0) {
            uint32_t ip = ntohl(client_addr.sin_addr.s_addr);
            printf("Received %d bytes from %d.%d.%d.%d:%d\n", ret,
                   (ip >> 24) & 0xFF,
                   (ip >> 16) & 0xFF,
                   (ip >> 8) & 0xFF,
                   ip & 0xFF,
                   ntohs(client_addr.sin_port));
            wolfIP_sock_sendto(g_wolfip, sockfd, buf, ret, 0,
                           (struct wolfIP_sockaddr*)&client_addr, client_len);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int wolfIP_Start_UDP_Echo(void) {
    BaseType_t ret;
    
    ret = xTaskCreate(UDP_Echo_Task,
                     "UDP_Echo",
                     WOLFIP_TASK_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 1,
                     NULL);
                     
    return (ret == pdPASS) ? 0 : -1;
}

int wolfIP_FreeRTOS_Start(void) {
    BaseType_t ret;
    
    if (!g_wolfip) {
        printf("wolfIP not initialized\n");
        return -1;
    }
    
    ret = xTaskCreate(wolfIP_NetworkTask,
                     "WolfIP_Net",
                     WOLFIP_TASK_STACK_SIZE,
                     NULL,
                     WOLFIP_TASK_PRIORITY,
                     &g_network_task);
                     
    return (ret == pdPASS) ? 0 : -1;
}
