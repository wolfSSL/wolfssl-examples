#include 			<stdio.h> 			/* standard in/out procedures */
#include 			<stdlib.h>			/* defines system calls */
#include 			<string.h>			/* necessary for memset */
#include			<netdb.h>
#include 			<sys/socket.h>		/* used for all socket calls */
#include 			<netinet/in.h>		/* used for sockaddr_in */
#include			<arpa/inet.h>
#include            <cyassl/ssl.h>      /* include the dtls library */
#include            <signal.h>          /* used to implement sigaction */

#define SERV_PORT   11111				/* define our server port number */
#define MSGLEN 		80  				/* limit incoming message size */

static int cleanup;                     /* handles shutdown */

void sig_handler(const int sig)                                                 
{                                                                               
    printf("\nSIGINT handled.\n");                                              
    cleanup = 1;                                                                
    return;                                                                     
}  

int
main(int argc, char** argv)
{
    /* CREATE THE SOCKET */

    struct sockaddr_in  servaddr; 		/* our server's address */
    struct sockaddr_in  cliaddr;		/* the client's address */
    int                 sockfd;			/* Initialize our socket */
    CYASSL*             cyasockfd;      /* sockfd for cyassl */
    socklen_t addrlen = sizeof(cliaddr);/* length of address' */
    int                 recvlen;		/* number of bytes recieved */
    int                 msgnum = 0;		/* the messages we reveive in order */
    char                buf[MSGLEN];    /* the incoming message */
    struct sigaction    act, oact;

    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    CyaSSL_Init();                      /* initialize cyassl */
    CYASSL_CTX* ctx;                    /* initialize the context */

    /* Create and initialize CYASSL_CTX structure */                            
    if ( (ctx = CyaSSL_CTX_new(CyaTLSv1_2_server_method())) == NULL){           
        fprintf(stderr, "CyaSSL_CTX_new error.\n");                             
        exit(EXIT_FAILURE);                                                     
    }                                                                           

    /* Load CA certificates into CYASSL_CTX */                                  
    if (CyaSSL_CTX_load_verify_locations(ctx,"certs/ca-cert.pem",0) !=       
            SSL_SUCCESS) {                                                      
        fprintf(stderr, "Error loading CA certs certs/ca-cert.pem, "                  
                "please check the file.\n");                                    
        exit(EXIT_FAILURE);                                                     
    }                                                                           

    /* Load server certificate into CYASSL_CTX */                               
    if (CyaSSL_CTX_use_certificate_file(ctx,"certs/server-cert.pem",         
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {                             
        fprintf(stderr, "Error loading server certs certs/server-cert.pem, "               
                "please check the file.\n");                                     
        exit(EXIT_FAILURE);                                                      
    }                                                                           

    /* Load server key into CYASSL_CTX */                                       
    if (CyaSSL_CTX_use_PrivateKey_file(ctx,"certs/server-key.pem",           
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {                             
        fprintf(stderr, "Error loading server keys certs/server-key.pem, "                
                "please check the file.\n");                                     
        exit(EXIT_FAILURE);                                                      
    }     

    /* create a UDP/IP socket */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create socket");
        return 0;
    }

    /* INADDR_ANY = IP address and socket =  11111, modify SERV_PORT to change */
    memset((char *)&servaddr, 0, sizeof(servaddr));

    /* Use Datagram Service */
    servaddr.sin_family 	 = AF_INET;

    /* host-to-network-long conversion (htonl) */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* host-to-network-short conversion (htons) */
    servaddr.sin_port 		 = htons(SERV_PORT);


    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    /* loop, listen for client, print received, reply to client */
    while(cleanup != 1) {
        printf("waiting for client message on port %d\n", SERV_PORT);
        recvlen = CyaSSL_read(cyasockfd, buf, MSGLEN);
        printf("heard %d bytes\n", recvlen);
        if (recvlen > 0) {
            buf[recvlen] = 0;
            printf("I heard this: \"%s\"\n", buf);
        }
        else
            printf("lost the connection to client\n");
        sprintf(buf, "Message #%d received\n", msgnum++);
        printf("reply sent \"%s\"\n", buf);
        if (CyaSSL_write(cyasockfd, buf, strlen(buf)) < 0)
            perror("sendto");
        /* continues to loop, use "Ctrl+C" to terminate listening */
    }
}
