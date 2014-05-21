#include 			<stdio.h> 			/* standard in/out procedures */
#include 			<stdlib.h>			/* defines system calls */
#include 			<string.h>			/* necessary for memset */
#include			<netdb.h>
#include 			<sys/socket.h>		/* used for all socket calls */
#include 			<netinet/in.h>		/* used for sockaddr_in */
#include			<arpa/inet.h>


#define SERV_PORT 	11111				/* define our server port number */
#define MSGLEN 		80  				/* limit incoming message size */

int
dtle_handle_read(struct dtls_context_t *ctx) {
    int *fd;
    session_t session;
    static uint8 buf[DTLS_MAX_BUF];
    int len;

    fd = dtls_get_app_data(ctx);

    assert(fd);

    session.size = sizeof(session.addr);
    len = recvfrom(*fd, buf, sizeof(buf), 0, &session.addr.sa,
            &session.size);

    return len < 0 ? len : dtls_handle_message(ctx, &session, buf, len);
}


int
read_from_peer(struct dtls_context_t *ctx, session_t *session, uint8 *data, size_t len) {
return dtls_write(ctx, session, data, len);
}


int
send_to_peer(struct dtls_context_t *ctx, session_t *session, uint8 *data, size_t len) {
    return sendto(fd, data, len, MSG_DONTWAIT, &session->addr.sa, session->size);
}

int get_key(struct dtls_context_t *ctx, const session_t *session, 
        const unsigned char *id, size_t id_len,
        const dtls_key_t **result){
    static const dtls_key_t psk = {
        .type = DTLS_KEY_PSK,
        .key.psk.id = (unsigned char *)"my identity", 
        .key.psk.id_length = 11,
        .key.psk.key = (unsigned char *)"secret", 
        .key.psk.key_length = 6
    };

    *result = &psk;
    return 0;
}

int 
handle_event(struct dtls_context_t *ctx, session_t *session, 
        dtls_alert_level_t level, unsigned short code) {
    /* do something with event */
        return 0;
}









    int
main(int argc, char** argv)
{
    /* CREATE THE SOCKET */

    struct sockaddr_in servaddr, cliaddr; 
    socklen_t addrlen = sizeof(cliaddr);
    char buf[MSGLEN];
    dtls_context_t* the_context = NULL;
    int fd, result;


    static dtls_handler_t cb = {
        .write = send_to_peer,
        .read = read_from_peer,
        .event = NULL,
        .get_key = get_key
    };


    /* create a UDP/IP socket */

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0 || bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        exit(-1);

    the_context = dtls_new_context(&fd);
    dtls_set_handler(the_context, &cb);

    while (1) {
        /* initialize fd_set rfds and timeout */
        result = select(fd+1, &rfds, NULL, 0, NULL);

        if (FD_ISSET(fd, &rfds))
            dtls_handle_read(the_context);
    }
    dtls_free_context(the_context);
}
