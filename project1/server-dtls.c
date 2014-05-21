#include 			<stdio.h> 			/* standard in/out procedures */
#include 			<stdlib.h>			/* defines system calls */
#include 			<string.h>			/* necessary for memset */
#include			<netdb.h>
#include 			<sys/socket.h>		/* used for all socket calls */
#include 			<netinet/in.h>		/* used for sockaddr_in */
#include			<arpa/inet.h>
#include            <cyassl/ssl.h>
#include            <cyassl/test.h>
#include            <errno.h>

#define SERV_PORT 	11111				/* define our server port number */
#define MSGLEN      4096

static int cleanup;		/* To handle shutdown */

void sig_handler(const int sig) 
{
    printf("\nSIGINT handled.\n");
    cleanup = 1;
    exit(0);
}

int
main(int argc, char** argv)
{
    /* CREATE THE SOCKET */

    struct sockaddr_in servaddr; 		/* our server's address */
    struct sockaddr_in cliaddr;			/* the client's address */
    int listenfd; 						/* Initialize our socket */
    socklen_t clilen;                   /* length of address' */
    int recvlen;                        /* length of message */
//    int connfd;                         /* the connection the client makes */
    char buff[MSGLEN];			        /* the incoming message */
    struct sigaction	act, oact;		/* structures for signal handling */

    /* Define a signal handler for when the user closes the program
       with Ctrl-C. Also, turn off SA_RESTART so that the OS doesn't 
       restart the call to accept() after the signal is handled. */

    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, &oact);

    CyaSSL_Init();      // Initialize CyaSSL
    CYASSL_CTX* ctx;

    /* Create and initialize CYASSL_CTX structure */
    if ( (ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL){
        fprintf(stderr, "CyaSSL_CTX_new error.\n");
        exit(EXIT_FAILURE);
    }

    /* Load CA certificates into CYASSL_CTX */
    if (CyaSSL_CTX_load_verify_locations(ctx,"certs/ca-cert.pem",0) != 
            SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/ca-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }

    /* Load server certificate into CYASSL_CTX */
    if (CyaSSL_CTX_use_certificate_file(ctx,"certs/server-cert.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-cert.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }

    /* Load server key into CYASSL_CTX */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx,"certs/server-key.pem", 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading certs/server-key.pem, "
                "please check the file.\n");
        exit(EXIT_FAILURE);
    }

    /* create a UDP/IP socket */

    if((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create socket");
        return 0;
    }

    /*	printf("created socket descriptor = %d\n", sockfd);  */

    /* INADDR_ANY=IP address and socket =  11111, modify SERV_PORT to change */
    memset((char *)&servaddr, 0, sizeof(servaddr));


    servaddr.sin_family = AF_INET;

    /* host-to-network-long conversion (htonl) */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* host-to-network-short conversion (htons) */
    servaddr.sin_port = htons(SERV_PORT);


    /* Eliminate socket already in use error */
    int res, on = 1;
    socklen_t len = sizeof(on);
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if(res<0){
        err_sys("setsockopt SO_REUSEADDR failed\n");
    }
    
    /* bind the socket */
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    /* loop, listen for client, print received, reply to client */
    while (cleanup != 1) {
        
        printf("waiting for client message on port %d\n", SERV_PORT);

        CYASSL*                 ssl;    /* initialize arg */
        clilen =    sizeof(cliaddr);    /* set clilen to |cliaddr| */
        unsigned char       b[1500];    
        int                       n;      

        n = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &clilen);

        printf("connection attempt made.\n");
        if(n > 0) {
            if(connect(listenfd, (const struct sockaddr*)&cliaddr,
                        sizeof(cliaddr)) != 0)
                err_sys("udp connect failed\n");

        }
//        else
//            err_sys("recvfrom failed\n");

        //        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        //
        //        if ( connfd == -1 ){
        //            printf("connfd = %d\n", connfd);
        //            printf("accept error\n");
        //        }

        else{

            printf("connected!\n");

            /* Create the CYASSL Object */
            if((ssl = CyaSSL_new(ctx))== NULL){
                fprintf(stderr, "CyaSSL_new error.\n");
                exit(EXIT_FAILURE);
            }

            /* set the session ssl to clientfd */
            CyaSSL_set_fd(ssl, listenfd);


            for(;;){
                if (CyaSSL_accept(ssl) != SSL_SUCCESS) {
                    int err = CyaSSL_get_error(ssl, 0);
                    char buffer[CYASSL_MAX_ERROR_SZ];
                    printf("error = %d, %s\n", err, CyaSSL_ERR_error_string(err, buffer));
                    err_sys("SSL_accept failed\n");
                }


                if( (recvlen = CyaSSL_read(ssl, buff, MSGLEN)) > 0){
                    printf("heard %d bytes\n", recvlen);



                    if (recvlen > 0) {
                        buff[recvlen] = 0;
                        printf("I heard this: \"%s\"\n", buff);
                    }

                    if (recvlen < 0){
                        int readErr = CyaSSL_get_error(ssl, 0);
                        if(readErr != SSL_ERROR_WANT_READ)
                            err_sys("SSL_read failed");
                    }
                    if (CyaSSL_write(ssl, buff, strlen(buff)) < 0){
                        perror("sendto");
                        printf("reply sent \"%s\"\n", buff);
                    }
                    /* continues to loop, use "Ctrl+C" to terminate listening */
                }
                else{
                    printf("lost the connection to client\n");
                    break;
                }

            }
        }

    }
    return(0);
}

