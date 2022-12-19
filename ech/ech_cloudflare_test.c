/* ech_cloudflare_test.c
 *
 * Copyright (C) 2006-2021 wolfSSL Inc.
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
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/test.h>
#include <errno.h>
#define SERV_PORT 443

#ifdef HAVE_ECH
int main(void)
{
  int ret;
  byte rd_buf[512];
  int sockfd;
  WOLFSSL_CTX* ctx;
  WOLFSSL* ssl;
  WOLFSSL_METHOD* method;
  struct  sockaddr_in servAddr;
  const char message[] =
	"GET /cdn-cgi/trace/ HTTP/1.1\r\n"
	"Host: crypto.cloudflare.com\r\n"
	"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:105.0) Gecko/20100101 Firefox/105.0\r\n"
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
	"Accept-Language: en-US,en;q=0.5\r\n"
	"Referer: https://www.google.com/\r\n"
	"DNT: 1\r\n"
	"Connection: keep-alive\r\n"
	"Upgrade-Insecure-Requests: 1\r\n"
	"Sec-Fetch-Dest: document\r\n"
	"Sec-Fetch-Mode: navigate\r\n"
	"Sec-Fetch-Site: cross-site\r\n"
	"Pragma: no-cache\r\n"
	"Cache-Control: no-cache\r\n"
	"\r\n";
  const char ip_string[] = "162.159.137.85";
  uint8_t ech_configs[72];
  uint32_t ech_configs_len = 72;

  /* this first tls connection is only used to get the retry configs */
  /* these configs can also be retrieved from DNS */

  	/* create and set up socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(SERV_PORT);
  // set the ip string to the cloudflare server
  servAddr.sin_addr.s_addr = inet_addr( ip_string );

  	/* connect to socket */
  connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));

  	/* initialize wolfssl library */
  wolfSSL_Init();
  method = wolfTLSv1_3_client_method(); /* use TLS v1.3 */

  	/* make new ssl context */
  if ( (ctx = wolfSSL_CTX_new(method)) == NULL) {
    	err_sys("wolfSSL_CTX_new error");
  }

  /* set the server name we want to connect to */
  ret = wolfSSL_CTX_UseSNI( ctx, WOLFSSL_SNI_HOST_NAME, "crypto.cloudflare.com", strlen("crypto.cloudflare.com") );

  if ( ret != WOLFSSL_SUCCESS ) {
    printf( "%d", ret );
    err_sys("wolfSSL_CTX_UseSNI error");
  }

  	/* make new wolfSSL struct */
  if ( (ssl = wolfSSL_new(ctx)) == NULL) {
    err_sys("wolfSSL_new error");
  }

  	/* Add cert to ctx */
  if (wolfSSL_CTX_load_verify_locations(ctx, "cert.pem", 0) !=
    SSL_SUCCESS) {
   	err_sys("Error loading ca-cert.pem");
  }

  	/* Connect wolfssl to the socket, server, then send message */
  wolfSSL_set_fd(ssl, sockfd);

  /* this connect will send a grease ech and get the retry configs back */
  ret = wolfSSL_connect(ssl);

  if (ret != SSL_SUCCESS) {
  	printf( "%d %d\n", ret, wolfSSL_get_error( ssl, ret ) );
   	err_sys("Error wolfSSL_connect");
  }

  /* retrieve the retry configs sent by the server */
  ret = wolfSSL_GetEchConfigs(ssl, ech_configs, &ech_configs_len);

  if (ret != SSL_SUCCESS) {
  	printf( "%d %d\n", ret, wolfSSL_get_error( ssl, ret ) );
   	err_sys("Error wolfSSL_get_ech_configs");
  }

  	/* frees all data before client termination */
  wolfSSL_free(ssl);
  close(sockfd);

  /* now we create a new connection that will send the real ech */

  	/* create and set up socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(SERV_PORT);
  // set the ip string to the cloudflare server
  servAddr.sin_addr.s_addr = inet_addr( ip_string );

  	/* connect to socket */
  connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));

  	/* make new wolfSSL struct */
  if ( (ssl = wolfSSL_new(ctx)) == NULL) {
   	err_sys("wolfSSL_new error");
  }

  // set the ech configs taken from dns
  ret = wolfSSL_SetEchConfigs(ssl, ech_configs, ech_configs_len);

  if ( ret != WOLFSSL_SUCCESS ) {
	  err_sys("wolfSSL_set_ech_configs error");
  }

  	/* Connect wolfssl to the socket, server, then send message */
  wolfSSL_set_fd(ssl, sockfd);

  /* this connect will send the real ech */
  ret = wolfSSL_connect(ssl);

  if (ret != SSL_SUCCESS) {
  	printf( "%d %d\n", ret, wolfSSL_get_error( ssl, ret ) );
   	err_sys("Error wolfSSL_connect");
  }

  wolfSSL_write(ssl, message, strlen(message));

  do
  {
    ret = wolfSSL_read( ssl, rd_buf, sizeof(rd_buf) );
    printf( "%.*s", ret, rd_buf );
  /* read until the chunk size is 0 */
  } while ( rd_buf[0] != '0' );

  wolfSSL_free(ssl);

  wolfSSL_CTX_free(ctx);
  wolfSSL_Cleanup();

  return 0;
}
#else
int main(void)
{
    printf("Please build wolfssl with ./configure --enable-ech\n");
    return 0;
}
#endif
