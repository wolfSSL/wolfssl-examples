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

    close(sock_fd);
    return 0;
}

static int tap_poll(struct ll *ll, void *buf, uint32_t len) {
    struct pollfd pfd;
    int ret;

    pfd.fd = tap_fd;
    pfd.events = POLLIN;
    ret = poll(&pfd, 1, 1); /* Short timeout */

    if (ret < 0) {
        perror("poll");
        return -1;
    }
    if (ret == 0) {
        return 0;
    }

    return read(tap_fd, buf, len);
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
