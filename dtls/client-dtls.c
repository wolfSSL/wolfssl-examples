#include <cyassl/ssl.h>
#include <cyassl/options.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE   4096
#define SERV_PORT 11111 

/* Send and receive function */
void DatagramClient (CYASSL* ssl) 
{
    int  n = 0;
    char m, sendLine[MAXLINE], recvLine[MAXLINE - 1];

    while (fgets(sendLine, MAXLINE, stdin) != NULL) {
    
       if ( ( CyaSSL_write(ssl, sendLine, strlen(sendLine))) != 
	      strlen(sendLine)) {
            printf("SSL_write failed");
        }

       n = CyaSSL_read(ssl, recvLine, sizeof(recvLine)-1);
       
       if (n < 0) {
            int readErr = CyaSSL_get_error(ssl, 0);
	        if(readErr != SSL_ERROR_WANT_READ) {
		        printf("CyaSSL_read failed");
            }
       }

        recvLine[n] = '\0';  
        fputs(recvLine, stdout);
    }
}

int main (int argc, char** argv)  
{
    int     	sockfd = 0;
    struct  	sockaddr_in servAddr;
    CYASSL* 	ssl = 0;
    CYASSL_CTX* ctx = 0;
    char        cert_array[]  = "../cyassl/certs/ca-cert.pem";
    char*       certs = cert_array;

    if (argc != 2) { 
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    CyaSSL_Init();
    /* CyaSSL_Debugging_ON(); */
   
    if ( (ctx = CyaSSL_CTX_new(CyaDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        return 1;
    }

    if (CyaSSL_CTX_load_verify_locations(ctx, certs, 0) 
	    != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }

    ssl = CyaSSL_new(ctx);
    if (ssl == NULL) {
    	printf("unable to get ssl object");
        return 1;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servAddr.sin_addr) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    CyaSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));
    
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
       printf("cannot create a socket."); 
       return 1;
    }
    CyaSSL_set_fd(ssl, sockfd);
    if (CyaSSL_connect(ssl) != SSL_SUCCESS) {
	    int err1 = CyaSSL_get_error(ssl, 0);
	    printf("err = %d, %d\n", err1, CyaSSL_ERR_reason_error_string(err1));
	    printf("SSL_connect failed");
        return 1;
    }
 
    DatagramClient(ssl);

    CyaSSL_shutdown(ssl);
    CyaSSL_free(ssl);
    close(sockfd);
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();

    return 0;
}

