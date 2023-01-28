/* client-tls-smtp-starttls.c
 *
 * Copyright (C) 2006-2023 wolfSSL Inc.
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

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/* socket includes */
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

/* wolfSSL */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/coding.h>

/* smtp starttls commands */
const char* starttlsCmd[21] = {
    "220",
    "EHLO mail.example.com\r\n",
    "250",
    "STARTTLS\r\n",
    "220",
    "AUTH LOGIN\r\n",
    "334",
    "334",
    "235",
    "MAIL FROM:<",
    "250",
    "RCPT TO:<",
    "250",
    "DATA\r\n",
    "354",
    "Subject: ",
    "To: ",
    "From: ",
    "250",
    "QUIT\r\n",
    "221"
};

int main(int argc, char** argv)
{
    int                sockfd;
    struct addrinfo hints,*res;
    char               buff[512], plain[512];
    size_t             len;
    int                ret;
    word32             outLen;

    /* declare wolfSSL objects */
    WOLFSSL_CTX* ctx;
    WOLFSSL*     ssl;

    /* Check for proper calling convention */
    if (argc != 3) {
        printf("usage: %s <SERVER_NAME> <CERT_FILE>\n", argv[0]);
        return 0;
    }

    /* Initialize the addrinfo struct with zero */
    memset(&hints, 0, sizeof(hints));

    /* Fill in the addrinfo struct */
    hints.ai_family = AF_INET;       /* using IPv4 */
    hints.ai_socktype = SOCK_STREAM; /* means TCP socket */
    char *service = "587";         /* use port 587 as a default */

    /* Get a Domain IP address */
    if(getaddrinfo(argv[1], service, &hints, &res) != 0){
        fprintf(stderr, "ERROR: failed to get the server ip\n");
        ret = -1;
        goto end;
    }

    /* Create a socket that uses an internet IPv4 address,
     * Sets the socket to be stream based (TCP),
     * 0 means choose the default protocol. */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
    }
    /* Free a list pointed by res */
    freeaddrinfo(res);
    /* Connect to the server */
    if ((ret = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
        fprintf(stderr, "ERROR: failed to connect\n");
        goto end;
    }

    /* S: 220 <host> SMTP service ready */
    memset(buff, 0, sizeof(buff));
    if (recv(sockfd, buff, sizeof(buff)-1, 0) < 0){
        fprintf(stderr, "failed to read STARTTLS command\n");
        goto end;
    }

    if ((!strncmp(buff, starttlsCmd[0], strlen(starttlsCmd[0]))) &&
        (buff[strlen(starttlsCmd[0])] == ' ')) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "incorrect STARTTLS command received\n");
        goto end;
    }

    /* C: EHLO mail.example.com */
    if (send(sockfd, starttlsCmd[1], (int)strlen(starttlsCmd[1]), 0) !=
              (int)strlen(starttlsCmd[1])){
        fprintf(stderr, "failed to send STARTTLS EHLO command\n");
        goto end;
    }

    /* S: 250 <host> offers a warm hug of welcome */
    memset(buff, 0, sizeof(buff));
    if (recv(sockfd, buff, sizeof(buff)-1, 0) < 0){
        fprintf(stderr, "failed to read STARTTLS command\n");
        goto end;
    }

    if ((!strncmp(buff, starttlsCmd[2], strlen(starttlsCmd[2]))) &&
        (buff[strlen(starttlsCmd[2])] == '-')) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "incorrect STARTTLS command received\n");
        goto end;
    }

    /* C: STARTTLS */
    if (send(sockfd, starttlsCmd[3], (int)strlen(starttlsCmd[3]), 0) !=
              (int)strlen(starttlsCmd[3])) {
        fprintf(stderr, "failed to send STARTTLS command\n");
        goto end;
    }

    /* S: 220 Go ahead */
    memset(buff, 0, sizeof(buff));
    if (recv(sockfd, buff, sizeof(buff)-1, 0) < 0){
        fprintf(stderr, "failed to read STARTTLS command\n");
        goto end;
    }
    buff[sizeof(buff)-1] = '\0';

    if ((!strncmp(buff, starttlsCmd[4], strlen(starttlsCmd[4]))) &&
        (buff[strlen(starttlsCmd[4])] == ' ')) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "incorrect STARTTLS command received, expected 220\n");
        goto end;
    }

    /*---------------------------------*/
    /* Start of wolfSSL initialization and configuration */
    /*---------------------------------*/
    /* Initialize wolfSSL */
    if ((ret = wolfSSL_Init()) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize the library\n");
        goto socket_cleanup;
    }

    /* Create and initialize WOLFSSL_CTX */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL_CTX\n");
        ret = -1;
        goto socket_cleanup;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((ret = wolfSSL_CTX_load_verify_locations(ctx, argv[2], NULL))
         != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to load %s, please check the file.\n",
                argv[2]);
        goto ctx_cleanup;
    }

    /* Create a WOLFSSL object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "ERROR: failed to create WOLFSSL object\n");
        ret = -1;
        goto ctx_cleanup;
    }

    /* Attach wolfSSL to the socket */
    if ((ret = wolfSSL_set_fd(ssl, sockfd)) != WOLFSSL_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to set the file descriptor\n");
        goto cleanup;
    }

    /* Connect to wolfSSL on the server side */
    if ((ret = wolfSSL_connect(ssl)) != SSL_SUCCESS) {
        fprintf(stderr, "ERROR: failed to connect to wolfSSL\n");
        goto cleanup;
    }

    /* Send "AUTH LOGIN\r\n" to the server */
    len = strlen(starttlsCmd[5]);
    if ((ret = wolfSSL_write(ssl, starttlsCmd[5], len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[6], strlen(starttlsCmd[6]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Get the mail address */
    printf("Mail Address: ");
    memset(plain, 0, sizeof(plain));
    if (fgets(plain, sizeof(plain), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get mail address.\n");
        ret = -1;
        goto cleanup;
    }
    /* Get the right mail address length */
    for(len=0; len<sizeof(plain); len++){
        if(plain[len] == '\n') break;
    }

    /* Encode the mail to Base64 */
    outLen = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    if(Base64_Encode((unsigned char*) plain, len, (unsigned char*) buff, &outLen) !=0){
        fprintf(stderr, "ERROR: failed to encode the mail address.\n");
        ret = -1;
        goto cleanup;
    }


    /*Change the line end to CRLF */
    strcpy(buff+outLen-1, "\r\n");

    /* Send encoded email address to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, outLen+1)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[7], strlen(starttlsCmd[7]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Get the password for mail account */
    printf("Password: ");
    memset(plain, 0, sizeof(plain));
    if (fgets(plain, sizeof(plain), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get password\n");
        ret = -1;
        goto cleanup;
    }
    /* Get the right password length */
    for(len=0; len<sizeof(plain); len++){
        if(plain[len] == '\n') break;
    }

    /* Encode the password to Base64 */
    outLen = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    if(Base64_Encode((unsigned char*) plain, len, (unsigned char*) buff, &outLen) !=0){
        fprintf(stderr, "ERROR: failed to encode the mail address.\n");
        ret = -1;
        goto cleanup;
    }

    /* Change the line end to CRLF */
    strcpy(buff+outLen-1, "\r\n");

    /* Send the encoded password to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, outLen+1)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[8], strlen(starttlsCmd[8]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Get the sender mail address */
    printf("Mail From: ");
    memset(buff, 0, sizeof(buff));
    strcpy(buff, starttlsCmd[9]);
    if (fgets(buff+strlen(starttlsCmd[9]), sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get sender mail address.\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff)-1, ">\r\n");
    printf("%s\n", buff);

    /* Send the sender mail address to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[10], strlen(starttlsCmd[10]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Get the right receiver mail address */
    printf("RCPT to: ");
    memset(buff, 0, sizeof(buff));
    strcpy(buff, starttlsCmd[11]);
    if (fgets(buff+strlen(starttlsCmd[11]), sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff)-1, ">\r\n");
    printf("%s\n", buff);

    /* Send the receiver mail address to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[12], strlen(starttlsCmd[12]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }
    /* Send "DATA\r\n" to the server */
    memset(buff, 0, sizeof(buff));
    len = strlen(starttlsCmd[13]);
    if ((ret = wolfSSL_write(ssl, starttlsCmd[13], len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[14], strlen(starttlsCmd[14]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Compose the mail */
    /* Get the Subject */
    printf("Subject: ");
    memset(buff, 0, sizeof(buff));
    strcpy(buff, starttlsCmd[15]);
    if (fgets(buff+strlen(starttlsCmd[15]), sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get the mail subject.\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff), "\r\n");

    /* Send the mail Subject to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send the mail subject.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Receiver mail address */
    printf("To: ");
    memset(buff, 0, sizeof(buff));
    strcpy(buff, starttlsCmd[16]);
    if (fgets(buff+strlen(starttlsCmd[16]), sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff), "\r\n");

    /* Send the receiver mail address to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send the receiver mail address.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Sender mail address */
    printf("From: ");
    memset(buff, 0, sizeof(buff));
    strcpy(buff, starttlsCmd[17]);
    if (fgets(buff+strlen(starttlsCmd[17]), sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message for server\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff), "\r\n");

    /* Send the sender mail address to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send the sender mail address.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* main message */
    printf("main message: ");
    memset(buff, 0, sizeof(buff));
    if (fgets(buff, sizeof(buff), stdin) == NULL) {
        fprintf(stderr, "ERROR: failed to get message.\n");
        ret = -1;
        goto cleanup;
    }

    strcpy(buff+strlen(buff), "\r\n");

    /* Send the main message to the server */
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send message.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Notify the end of the mail input to the server */
    memset(buff, 0, sizeof(buff));
    strcpy(buff, ".\r\n");
    len = strnlen(buff, sizeof(buff));
    if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[18], strlen(starttlsCmd[18]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }

    /* Send "QUIT\r\n" to the server */
    memset(buff, 0, sizeof(buff));
    len = strlen(starttlsCmd[19]);
    if ((ret = wolfSSL_write(ssl, starttlsCmd[19], len)) != len) {
        fprintf(stderr, "ERROR: failed to send command.\n");
        fprintf(stderr, "%d bytes of %d bytes were sent", ret, (int) len);
        goto cleanup;
    }

    /* Read the server data into our buff array */
    memset(buff, 0, sizeof(buff));
    if ((ret = wolfSSL_read(ssl, buff, sizeof(buff)-1)) == -1) {
        fprintf(stderr, "ERROR: failed to read\n");
        goto cleanup;
    }
    /* Compare if the response is right code or not */
    if (!strncmp(buff, starttlsCmd[20], strlen(starttlsCmd[20]))) {
        printf("%s\n", buff);
    } else {
        fprintf(stderr, "ERROR: incorrect command received\n");
        printf("%s\n", buff);
        goto cleanup;
    }



    /* Cleanup and return */
cleanup:
    wolfSSL_free(ssl);      /* Free the wolfSSL object                  */
ctx_cleanup:
    wolfSSL_CTX_free(ctx);  /* Free the wolfSSL context object          */
    wolfSSL_Cleanup();      /* Cleanup the wolfSSL environment          */
socket_cleanup:
    close(sockfd);          /* Close the connection to the server       */
end:
    return ret;               /* Return reporting a success               */
}
