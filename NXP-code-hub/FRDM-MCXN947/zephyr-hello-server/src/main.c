/* main.c
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* wolfSSL Includes Start	*/
#include "user_settings.h" 		//For wolfssl zephyr configuration (Bunch of Defines)
#include <wolfssl/ssl.h>		//Basic functionality for TLS
#include <wolfssl/certs_test.h> //Needed for Cert Buffers
#include <wolfssl/wolfcrypt/hash.h>
/* wolfSSL Includes End		*/

/* Standard Packages Start	*/
#include <stdio.h>
#include <time.h>
/* Standard Packages End	*/

/* Zephyr Includes Start	*/
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/net_ip.h>
/* Zephyr Includes End 		*/

/* Program Defines Start 	*/

#define DEFAULT_PORT 11111 	//Define's the port we want to use for the network

#define LOCAL_DEBUG 0		//Use for wolfSSL's internal Debugging



/* Use DHCP auto ip assignment or static assignment */
#undef	DHCP_ON
#define DHCP_ON 0 	// Set to true (1) if you want auto assignment ip, set false (0) for staticly define.
					// Make sure to avoid IP conflicts on the network you assign this to, check the defaults before using.
					// If unsure leave DHCP_ON set to 1
 
#if DHCP_ON == 0
	//TODO Setup static defines for ip
/* Define Static IP, Gateway, and Netmask */
	#define STATIC_IPV4_ADDR  "192.168.1.70"
	#define STATIC_IPV4_GATEWAY "192.168.1.1"
	#define STATIC_IPV4_NETMASK "255.255.255.0"
#endif

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000
/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* Password attempts */
#define MAX_PASSWD_RETRY 3

#define EXAMPLE_BUFFER_SZ 4096
#define EXAMPLE_KEYLOAD_BUFFER_SZ 1200



/* Program Defines End		*/


/* Global Variables/Structs Start 	*/

//static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Gloabl Variables/Starts End 		*/


/* */



int startNetwork(){

	struct net_if *iface = net_if_get_default();
	char buf[NET_IPV4_ADDR_LEN];

	#if DHCP_ON == 0
		struct in_addr addr, netmask, gw;
	#endif

	if (!(iface)) { //See if a network interface (ethernet) is avaliable
		printf("No network interface determined");
		return 1;
	}

	if (net_if_flag_is_set(iface, NET_IF_DORMANT)) {
		printf("Waiting on network interface to be avaliable");
		while(!net_if_is_up(iface)){
			k_sleep(K_MSEC(100));
		}
	}

	#if DHCP_ON == 1
    	printf("\nStarting DHCP to obtain IP address\n");
    	net_dhcpv4_start(iface);
    	(void)net_mgmt_event_wait_on_iface(iface, NET_EVENT_IPV4_DHCP_BOUND, NULL, NULL, NULL, K_FOREVER);
	#elif DHCP_ON == 0
		/* Static IP Configuration */
		if (net_addr_pton(AF_INET, STATIC_IPV4_ADDR, &addr) < 0 ||
			net_addr_pton(AF_INET, STATIC_IPV4_NETMASK, &netmask) < 0 ||
			net_addr_pton(AF_INET, STATIC_IPV4_GATEWAY, &gw) < 0) {
			printf("Invalid IP address settings.\n");
			return -1;
		}
		net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask);
		net_if_ipv4_set_gw(iface, &gw);
		net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);


	#else
		#error "Please set DHCP_ON to true (1) or false (2), if unsure set to true (1)"
	#endif


	/* Display IP address that was assigned when done */
	printf("IP Address is: %s", net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr, buf, sizeof(buf)));
	
	return 0;

}


int startServer(void)
{
    int                sockfd = SOCKET_INVALID;
    int                connd = SOCKET_INVALID;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[256];
    size_t             len;
    int                shutdown = 0;
    int                ret;
    const char*        reply = "I hear ya fa shizzle!\n";

    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL*     ssl = NULL;
    WOLFSSL_CIPHER* cipher;

    #if LOCAL_DEBUG
        wolfSSL_Debugging_ON();
    #endif

    wolfSSL_Init();

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("\nERROR: failed to create the socket\n");
        return 1;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_3_server_method());
    if (ctx == NULL) {
        printf("\nERROR: Failed to create WOLFSSL_CTX\n");
        return 1;
    }

    if (wolfSSL_CTX_use_certificate_chain_buffer_format(ctx,
                server_cert_der_2048, sizeof_server_cert_der_2048,
                WOLFSSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS){
        printf("\nERROR: Cannot load server cert buffer\n");
        return 1; 
    }

    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, server_key_der_2048,
            sizeof_server_key_der_2048, SSL_FILETYPE_ASN1) != WOLFSSL_SUCCESS){
        printf("\nERROR: Can't load server private key buffer");
        return 1;
    }

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET;             /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY;          /* from anywhere   */

    /* Bind the server socket to our port */
    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        printf("\nERROR: failed to bind\n");
        return 1;
    }

    /* Listen for a new connection, allow 5 pending connections */
    if (listen(sockfd, 5) == -1) {
        printf("\nERROR: failed to listen\n");
        return 1;
    } 

    printf("\nServer Started\n");

        /* Continue to accept clients until shutdown is issued */
    while (!shutdown) {
        printf("Waiting for a connection...\n");

        /* Accept client connections */
        if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {
            printf("\nERROR: failed to accept the connection\n");
            return 1;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            printf("\nERROR: failed to create WOLFSSL object\n");
            return 1;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, connd);

        /* Establish TLS connection */
        ret = wolfSSL_accept(ssl);
        if (ret != WOLFSSL_SUCCESS) {
            printf("\nERROR: wolfSSL_accept error = %d\n",
                wolfSSL_get_error(ssl, ret));
            return 1;
        }


        printf("Client connected successfully\n");

        cipher = wolfSSL_get_current_cipher(ssl);
        printf("SSL cipher suite is %s\n", wolfSSL_CIPHER_get_name(cipher));


        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
            printf("\nERROR: failed to read\n");
            return 1;
        }

        /* Print to stdout any data the client sends */
        printf("Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            printf("Shutdown command issued!\n");
            shutdown = 1;
        }



        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            printf("\nERROR: failed to write\n");
            return 1;
        }

        /* Notify the client that the connection is ending */
        wolfSSL_shutdown(ssl);
        printf("Shutdown complete\n");

        /* Cleanup after this connection */
        wolfSSL_free(ssl);      /* Free the wolfSSL object              */
        ssl = NULL;
        close(connd);           /* Close the connection to the client   */
    }

    return 0;
}



int main(void)
{
    printf("\nRunning wolfSSL example from the %s!\n", CONFIG_BOARD);
	
    /* Start up the network */
    if (startNetwork() != 0){
        printf("Network Initialization via DHCP Failed");
        return 1;
    }


    if (startServer() != 0){
        printf("Server has Failed!");
        return 1;
    }

    return 0;
}