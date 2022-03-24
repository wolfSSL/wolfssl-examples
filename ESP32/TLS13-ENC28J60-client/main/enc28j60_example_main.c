/* ENC28J60 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 *
 *  see https://www.wolfssl.com/docs/quickstart/
 **/

#define WOLFSSL_ESPIDF
#define WOLFSSL_ESPWROOM32
#define WOLFSSL_USER_SETTINGS

/* the usual suspects */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "enc28j60.h"
#include "driver/spi_master.h"

#include "nvs_flash.h"
#if ESP_IDF_VERSION_MAJOR >= 4
// #include "protocol_examples_common.h"
#endif

/* ESP specific */
#include "wifi_connect.h"

/* socket includes */
#include "lwip/netdb.h"
#include "lwip/sockets.h"

/* time */
#include  <lwip/apps/sntp.h>

/* wolfSSL */
#include <wolfssl/wolfcrypt/settings.h> // make sure this appears before any other wolfSSL headers
#include <wolfssl/ssl.h>

#ifdef WOLFSSL_TRACK_MEMORY
#include <wolfssl/wolfcrypt/mem_track.h>
#endif

/**
 ******************************************************************************
 ******************************************************************************
 ** USER APPLICATION SETTINGS BEGIN
 ******************************************************************************
 ******************************************************************************
 **/

#define DEFAULT_PORT                     11111

#define TLS_SMP_CLIENT_TASK_NAME         "tls_client_example"
#define TLS_SMP_CLIENT_TASK_WORDS        10240
#define TLS_SMP_CLIENT_TASK_PRIORITY     8

#define TLS_SMP_TARGET_HOST              "192.168.75.55"


/* include certificates. Note that there is an experiation date! 
 * 
 * See also https://github.com/wolfSSL/wolfssl/blob/master/wolfssl/certs_test.h
 
   for example:
     
    #define USE_CERT_BUFFERS_2048
    #include <wolfssl/certs_test.h>
*/
#include "embedded_CERT_FILE.h"
#include "embedded_CA_FILE.h"
#include "embedded_KEY_FILE.h"

static const char *TAG = "eth_example";

/* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
   02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
*/
uint8_t myMacAddress[] = {
    0x02,
    0x00,
    0x00,
    0x12,
    0x34,
    0x56
};

// see https://tf.nist.gov/tf-cgi/servers.cgi
const int NTP_SERVER_COUNT = 3;
const char* ntpServerList[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "utcnist.colorado.edu"
};
const char * TIME_ZONE = "PST-8";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

/* const char sndMsg[] = "GET /index.html HTTP/1.0\r\n\r\n"; */
const char sendMessage[] = "Hello World\n";
const int sendMessageSize = sizeof(sendMessage);

TickType_t DelayTicks = 5000 / portTICK_PERIOD_MS;
/**
 ******************************************************************************
 ******************************************************************************
 ** USER SETTINGS END
 ******************************************************************************
 ******************************************************************************
 **/
  

int tls_smp_client_task() {
    int ret = WOLFSSL_SUCCESS; /* assume success until proven wrong */
    int sockfd = 0; /* the socket that will carry our secure connection */
    struct sockaddr_in servAddr;
    const int BUFF_SIZE = 256;
    char buff[BUFF_SIZE];
    const char* targetServer = TLS_SMP_TARGET_HOST;
    size_t len; /* we'll be looking at the length of messages sent and received */
    struct hostent *hp;
    struct ip4_addr *ip4_addr;
    

    struct timeval tv_now;
    
    WOLFSSL_ENTER("tls_smp_client_task");

    /* see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#overview */
    // ret = gettimeofday(&tv_now, NULL);
    /* 0x0005dcacd95af765 */
    // int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
    
    // WOLFSSL_MSG(time_us) ;
    
    /* declare wolfSSL objects */
    WOLFSSL_CTX *ctx = NULL; /* the wolfSSL context object*/
    WOLFSSL *ssl = NULL; /* although called "ssl" is is the secure object for reading and writings data*/


#ifdef DEBUG_WOLFSSL
    WOLFSSL_MSG("Debug ON");
    wolfSSL_Debugging_ON();
    //ShowCiphers();
#endif
    
#ifndef WOLFSSL_TLS13
    ret = WOLFSSL_FAILURE;
    WOLFSSL_ERROR_MSG("ERROR: Example requires TLS v1.3.\n");
#endif /* WOLFSSL_TLS13 */
   
    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family = AF_INET; /* using IPv4      */
    servAddr.sin_port   = htons(DEFAULT_PORT); /* on DEFAULT_PORT */    

    
    
    ESP_LOGI(TAG, "get target IP address");

    hp = gethostbyname(TLS_SMP_TARGET_HOST);
    if (!hp) {
        ESP_LOGE(TAG, "Failed to get host name.");
        ip4_addr = NULL;
    }
    else {
        ip4_addr = (struct ip4_addr *)hp->h_addr;
        ESP_LOGI(TAG, IPSTR, IP2STR(ip4_addr));
    }
    

    if (*targetServer >= '1' && *targetServer <= '9') {
        /* Get the server IPv4 address from the command line call */
        WOLFSSL_MSG("inet_pton");
        if ((ret = inet_pton(AF_INET,
            TLS_SMP_TARGET_HOST,
            &servAddr.sin_addr)) != 1) {
            ESP_LOGE(TAG, "ERROR: invalid address ret=%d\n", ret);

            ret = WOLFSSL_FAILURE;
            WOLFSSL_ERROR_MSG("ERROR: invalid address\n");
        }
    }
    else {
        servAddr.sin_addr.s_addr = ip4_addr->addr;
    }
    
    /* 
    ***************************************************************************
    * Create a socket that uses an internet IPv4 address,
    * Sets the socket to be stream based (TCP),
    * 0 means choose the default protocol.
    * 
    *  #include <sys/socket.h>
    *
    *  int socket(int domain, int type, int protocol);  
    *  
    *  see: https://linux.die.net/man/3/socket
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /* Upon successful completion, socket() shall return 
         * a non-negative integer, the socket file descriptor.
        */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd > 0) {
            WOLFSSL_MSG("socket creation successful\n");
        }
        else {
            // TODO show errno 
            ret = WOLFSSL_FAILURE;
            WOLFSSL_ERROR_MSG("ERROR: failed to create a socket.\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("Skipping socket create.\n");
    }
    

    /* 
    ***************************************************************************
    *  Connect the TCP socket to the server (no encryption yet!) 
    *  
    *  #include <sys/socket.h>
    *  int connect(int socket, const struct sockaddr *address, socklen_t address_len);
    *  
    *  See https://linux.die.net/man/3/connect
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /*
         * Upon successful completion, socket() shall return a non-negative integer, 
         * the socket file descriptor. 
         * 
         * Upon successful completion, connect() shall return 0; otherwise, 
         * -1 shall be returned and errno set to indicate the error.
         */
        int connectResult = connect(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr));
        if (0 == connectResult) {
            WOLFSSL_MSG("sockfd connect successful\n");
        }
        else {
            // TODO show errno
            WOLFSSL_ERROR_MSG("ERROR: socket connect failed\n");
            ret = WOLFSSL_FAILURE;
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("Skipping socket connect.\n");
    }
    
    
    /* 
    ***************************************************************************
    * Initialize wolfSSL 
    * 
    *  WOLFSSL_API int wolfSSL_Init    (void)
    *
    *  Initializes the wolfSSL library for use. Must be called once per 
    *  application and before any other call to the library.
    *
    *  Returns
    *    SSL_SUCCESS  If successful the call will return.
    *    BAD_MUTEX_E  is an error that may be returned.
    *    WC_INIT_E    wolfCrypt initialization error returned.
    * 
    *  see: https://www.wolfssl.com/doxygen/group__TLS.html#gae2a25854de5230820a6edf16281d8fd7
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /* only proceed if the prior step was successful */
        WOLFSSL_MSG("calling wolfSSL_Init");
        ret = wolfSSL_Init();

        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_Init successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_Init failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("Skipping wolfSSL_Init\n");
    }

    
    /* 
    ***************************************************************************
    * Create and initialize WOLFSSL_CTX (aka the context)
    * 
    *  WOLFSSL_API WOLFSSL_CTX* wolfSSL_CTX_new    (WOLFSSL_METHOD *)
    * 
    *  This function creates a new SSL context, taking a desired 
    *  SSL/TLS protocol method for input.
    *
    *  Returns
    *    pointer If successful the call will return a pointer to the newly-created WOLFSSL_CTX.
    *    NULL upon failure.
    *
    *  Parameters
    *    method pointer to the desired WOLFSSL_METHOD to use for the SSL context. 
    *    This is created using one of the wolfSSLvXX_XXXX_method() functions to
    *    specify SSL/TLS/DTLS protocol level.
    * 
    *  see https://www.wolfssl.com/doxygen/group__Setup.html#gadfa552e771944a6a1102aa43f45378b5
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        WOLFSSL_METHOD* method = wolfTLSv1_3_client_method();
        if (method == NULL) {
            WOLFSSL_ERROR_MSG("ERROR : failed to get  wolfTLSv1_3_client_method.\n");
            ret = WOLFSSL_FAILURE;
        }
        else {
            ctx = wolfSSL_CTX_new(method);

            if (ctx == NULL) {
                WOLFSSL_ERROR_MSG("ERROR : failed to create WOLFSSL_CTX\n");
                ret = WOLFSSL_FAILURE;
            }
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_CTX_new\n");
    }
    
    
    /* 
    ***************************************************************************
    *  load CERT_FILE 
    *  
    *  
    *  WOLFSSL_API int wolfSSL_use_certificate_buffer (WOLFSSL * ,
    *                                                  const unsigned char * ,
    *                                                  long,
    *                                                  int      
    *                                                  )
    *  
    *  The wolfSSL_use_certificate_buffer() function loads a certificate buffer 
    *  into the WOLFSSL object. It behaves like the non-buffered version, only 
    *  differing in its ability to be called with a buffer as input instead of 
    *  a file. The buffer is provided by the in argument of size sz. 
    *  
    *  format specifies the format type of the buffer; SSL_FILETYPE_ASN1 or 
    *  SSL_FILETYPE_PEM. Please see the examples for proper usage.
    *  
    *  Returns
    *    SSL_SUCCESS      upon success.
    *    SSL_BAD_FILETYPE will be returned if the file is the wrong format.
    *    SSL_BAD_FILE     will be returned if the file doesn’t exist, can’t be read, or is corrupted.
    *    MEMORY_E         will be returned if an out of memory condition occurs.
    *    ASN_INPUT_E      will be returned if Base16 decoding fails on the file.
    *  
    *  Parameters
    *    ssl    pointer to the SSL session, created with wolfSSL_new().
    *    in     buffer containing certificate to load.
    *    sz     size of the certificate located in buffer.
    *    format format of the certificate to be loaded. Possible values are SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM.
    *  
    *
    *  Pay attention to expiration dates and the current date setting
    *  
    *  see https://www.wolfssl.com/doxygen/group__CertsKeys.html#gaf4e8d912f3fe2c37731863e1cad5c97e
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        WOLFSSL_MSG("Loading cert");
        ret = wolfSSL_CTX_use_certificate_buffer(ctx, 
            CERT_FILE, 
            sizeof_CERT_FILE(), 
            WOLFSSL_FILETYPE_PEM);

        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_CTX_use_certificate_buffer successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_CTX_use_certificate_buffer failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_CTX_use_certificate_buffer\n");
    }
        
    
    /* 
    ***************************************************************************
    *  Load client private key into WOLFSSL_CTX 
    *  
    *  wolfSSL_CTX_use_PrivateKey_buffer()
    *  
    *  WOLFSSL_API int wolfSSL_CTX_use_PrivateKey_buffer(WOLFSSL_CTX *,
    *                                                    const unsigned char *,
    *                                                    long,
    *                                                    int      
    *                                                   )
    *
    *  This function loads a private key buffer into the SSL Context. 
    *  It behaves like the non-buffered version, only differing in its 
    *  ability to be called with a buffer as input instead of a file. 
    *  
    *  The buffer is provided by the in argument of size sz. format 
    *  specifies the format type of the buffer; 
    *  SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM. 
    *  
    *  Please see the examples for proper usage.
    *
    *  Returns
    *    SSL_SUCCESS upon success
    *    SSL_BAD_FILETYPE will be returned if the file is the wrong format.
    *    SSL_BAD_FILE will be returned if the file doesn’t exist, can’t be read, or is corrupted.
    *    MEMORY_E will be returned if an out of memory condition occurs.
    *    ASN_INPUT_E will be returned if Base16 decoding fails on the file.
    *    NO_PASSWORD will be returned if the key file is encrypted but no password is provided.
    *
    *  Parameters
    *    ctx      pointer to the SSL context, created with wolfSSL_CTX_new().
    *             inthe input buffer containing the private key to be loaded.
    *    
    *    sz          the size of the input buffer.
    *    
    *    format  the format of the private key located in the input buffer(in). 
    *            Possible values are SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM.
    *
    *  see: https://www.wolfssl.com/doxygen/group__CertsKeys.html#ga71850887b87138b7c2d794bf6b1eafab
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx, 
                                                KEY_FILE, 
                                                sizeof_KEY_FILE(), 
                                                WOLFSSL_FILETYPE_PEM);
        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_CTX_use_PrivateKey_buffer successful\n");
        }
        else {
            /* TODO fetch and print expiration date since it is a common fail */
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_CTX_use_PrivateKey_buffer failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("Skipping wolfSSL_CTX_use_PrivateKey_buffer\n");
    }

    
    /* 
    ***************************************************************************
    *  Load CA certificate into WOLFSSL_CTX 
    * 
    *  wolfSSL_CTX_load_verify_buffer()
    *  WOLFSSL_API int wolfSSL_CTX_load_verify_buffer(WOLFSSL_CTX *,
    *                                                 const unsigned char *,
    *                                                 long,
    *                                                 int      
    *                                                )        
    *                                                
    *  This function loads a CA certificate buffer into the WOLFSSL Context. 
    *  It behaves like the non-buffered version, only differing in its ability 
    *  to be called with a buffer as input instead of a file. The buffer is 
    *  provided by the in argument of size sz. format specifies the format type 
    *  of the buffer; SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM. More than one 
    *  CA certificate may be loaded per buffer as long as the format is in PEM.
    *  
    *  Please see the examples for proper usage.
    *  
    *  Returns
    *  
    *    SSL_SUCCESS upon success
    *    SSL_BAD_FILETYPE will be returned if the file is the wrong format.
    *    SSL_BAD_FILE will be returned if the file doesn’t exist, can’t be read, or is corrupted.
    *    MEMORY_E will be returned if an out of memory condition occurs.
    *    ASN_INPUT_E will be returned if Base16 decoding fails on the file.
    *    BUFFER_E will be returned if a chain buffer is bigger than the receiving buffer.
    *    
    *  Parameters
    *  
    *    ctx    pointer to the SSL context, created with wolfSSL_CTX_new().
    *    in    pointer to the CA certificate buffer.
    *    sz    size of the input CA certificate buffer, in.
    *    format    format of the buffer certificate, either SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM.
    *
    * see https://www.wolfssl.com/doxygen/group__CertsKeys.html#gaa37539cce3388c628ac4672cf5606785
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CTX_load_verify_buffer(ctx, CA_FILE, sizeof_CA_FILE(), WOLFSSL_FILETYPE_PEM);
        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_CTX_load_verify_buffer successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_CTX_load_verify_buffer failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_CTX_load_verify_buffer\n");
    }
    
    
    /* 
    ***************************************************************************
    *  Create a WOLFSSL object 
    *  
    *  The wolfSSL_new() function creates a new SSL session, taking an already 
    *  created SSL context as input.
    *
    *  wolfSSL_new() Returns:
    *  
    *     If successful the call will return a pointer to the newly-created 
    *     wolfSSL structure.
    *     
    *     NULL Upon failure.
    *     
    *  Parameters: WOLFSSL_API WOLFSSL* wolfSSL_new(WOLFSSL_CTX *)
    *  
    *    ctx  pointer to the SSL context, created with wolfSSL_CTX_new().
    *
    *  See: https://www.wolfssl.com/doxygen/group__Setup.html#ga3b1873a50ef7fcee4e2cc8968c81b6c9
    *  
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ssl = wolfSSL_new(ctx);
        if (ssl == NULL) {
            WOLFSSL_ERROR_MSG("ERROR : failed to create WOLFSSL object\n");
            ret = WOLFSSL_FAILURE;
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_new\n");
    }


    /* 
    ***************************************************************************
    *  Attach wolfSSL to the socket using wolfSSL_set_fd()
    *  
    *  This function assigns a file descriptor (fd) as the input/output 
    *  facility for the SSL connection. Typically this will be a socket 
    *  file descriptor.
    *  
    *  wolfSSL_set_fd returns:
    *  
    *    SSL_SUCCESS upon success.
    *    Bad_FUNC_ARG upon failure.
    *    
    *  Parameters:
    *  
    *    ssl  pointer to the SSL session, created with wolfSSL_new().
    *    fd   file descriptor to use with SSL/TLS connection.
    *    
    *  see https://www.wolfssl.com/doxygen/group__Setup.html#ga4f23ec6e60cc92e0e899071653d3188b
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_set_fd(ssl, sockfd);
        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_set_fd successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_set_fd failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_set_fd\n");
    }


    /* 
    ***************************************************************************
    *  Connect to wolfSSL on the server side 
    *  
    *  This function is called on the client side and initiates an SSL/TLS 
    *  handshake with a server. When this function is called, the underlying 
    *  communication channel has already been set up. wolfSSL_connect() works 
    *  with both blocking and non-blocking I/O. When the underlying I/O is 
    *  non-blocking, wolfSSL_connect() will return when the underlying I/O 
    *  could not satisfy the needs of wolfSSL_connect to continue the 
    *  handshake. 
    *  
    *  In this case, a call to wolfSSL_get_error() will yield either 
    *  SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE. The calling process must 
    *  then repeat the call to wolfSSL_connect() when the underlying I/O 
    *  is ready and wolfSSL will pick up where it left off. 
    *  
    *  When using a non-blocking socket, nothing needs to be done, 
    *  but select() can be used to check for the required condition. 
    *  
    *  If the underlying I/O is blocking, wolfSSL_connect() will only return 
    *  once the handshake has been finished or an error occurred. wolfSSL takes 
    *  a different approach to certificate verification than OpenSSL does. 
    *  
    *  The default policy for the client is to verify the server, this means 
    *  that if you don't load CAs to verify the server you'll get a connect 
    *  error, unable to verify (-155). 
    *  
    *  If you want to mimic OpenSSL behavior of having SSL_connect succeed even 
    *  if verifying the server fails and reducing security you can do this by 
    *  calling: SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0); before 
    *  calling SSL_new(); Though it's not recommended.
    *  
    *  Returns:
    *  
    *    SSL_SUCCESS       If successful.
    *    SSL_FATAL_ERROR   will be returned if an error occurred. To get a more 
    *                      detailed error code, call wolfSSL_get_error().
    *    
    *  Parameters:
    *  
    *    ssl   a pointer to a WOLFSSL structure, created using wolfSSL_new().
    *
    *  see: https://www.wolfssl.com/doxygen/group__IO.html#ga5b8f41cca120758d1860c7bc959755dd
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_connect(ssl);
        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_connect successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_connect failed\n");
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("skipping wolfSSL_connect\n");
    }
    

    /*
    ***************************************************************************
    *  send the message to the server 
    *  
    *  The wolfSSL_write() function writes sz bytes from the buffer, data, to 
    *  the SSL connection, ssl. If necessary, wolfSSL_write() will negotiate an 
    *  SSL/TLS session if the handshake has not already been performed yet by 
    *  wolfSSL_connect() or wolfSSL_accept(). wolfSSL_write() works with both 
    *  blocking and non-blocking I/O. 
    *  
    *  When the underlying I/O is non-blocking, wolfSSL_write() will return 
    *  when the underlying I/O could not satisfy the needs of wolfSSL_write() 
    *  to continue. In this case, a call to wolfSSL_get_error() will yield 
    *  either SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE. The calling process 
    *  must then repeat the call to wolfSSL_write() when the underlying I/O is 
    *  ready. 
    *  
    *  If the underlying I/O is blocking, wolfSSL_write() will only return once 
    *  the buffer data of size sz has been completely written or an error 
    *  occurred.
    *  
    *  wolfSSL_write() Returns:
    *  
    *    >0 the number of bytes written upon success.
    *    
    *    0 will be returned upon failure. Call wolfSSL_get_error() for 
    *    the specific error code.
    *    
    *    SSL_FATAL_ERROR will be returned upon failure when either an error 
    *    occurred or, when using non-blocking sockets, the SSL_ERROR_WANT_READ 
    *    or SSL_ERROR_WANT_WRITE error was received and and the application
    *    needs to call wolfSSL_write() again. Use wolfSSL_get_error() 
    *    to get a specific error code.
    *
    *  Parameters: 
    *  
    *    ssl    pointer to the SSL session, created with wolfSSL_new().
    *    data    data buffer which will be sent to peer.
    *    sz    size, in bytes, of data to send to the peer (data).
    *    
    *  see: https://www.wolfssl.com/doxygen/group__IO.html#ga74b924a81e9efdf66d074690e5f53ef1
    *
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {

        memset(buff, 0, BUFF_SIZE);
    
        /* get the length of our message, never longer than the declared size */
        
        /* TODO check for zero length */
        
        len = strnlen(sendMessage, sendMessageSize);
        
        /* write the message over secure connection to the server */
        if (wolfSSL_write(ssl, sendMessage, len) == len) {

            WOLFSSL_MSG("wolfSSL_write message sent successfully:\n");
            WOLFSSL_MSG(sendMessage);
        }
        else {

            int err = 0;
            err = wolfSSL_get_error(ssl, 0);

            char err_buff[80];
            wolfSSL_ERR_error_string(err, err_buff);
            WOLFSSL_ERROR_MSG(err_buff);

            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_write FAILED.\n");
            ret = WOLFSSL_FAILURE;
        }
    }
    else {
        /* a prior error occured */
        WOLFSSL_ERROR_MSG("Skipping wolfSSL_write\n");
    }
   

    /* 
    ***************************************************************************
    * 
    *  Read the server data into our buff array 
    *  
    *  The wolfSSL_read() function reads sz bytes from the SSL session (ssl) 
    *  internal read buffer into the buffer data. The bytes read are removed 
    *  from the internal receive buffer. If necessary wolfSSL_read() will 
    *  negotiate an SSL/TLS session if the handshake has not already been 
    *  performed yet by wolfSSL_connect() or wolfSSL_accept(). The SSL/TLS 
    *  protocol uses SSL records which have a maximum size of 16kB (the max 
    *  record size can be controlled by the MAX_RECORD_SIZE define in 
    *  <wolfssl_root>/wolfssl/internal.h). 
    *  
    *  As such, wolfSSL needs to read an entire SSL record internally before 
    *  it is able to process and decrypt the record. Because of this, a call 
    *  to wolfSSL_read() will only be able to return the maximum buffer size 
    *  which has been decrypted at the time of calling. There may be additional
    *  not-yet-decrypted data waiting in the internal wolfSSL receive buffer 
    *  which will be retrieved and decrypted with the next call to 
    *  wolfSSL_read(). 
    *  
    *  If sz is larger than the number of bytes in the internal read buffer, 
    *  SSL_read() will return the bytes available in the internal read buffer. 
    *  If no bytes are buffered in the internal read buffer yet, a call to 
    *  wolfSSL_read() will trigger processing of the next record.

    *  Returns
    *  
    *   >0 the number of bytes read upon success.
    *   
    *   0 will be returned upon failure. This may be caused by a either a clean
    *   (close notify alert) shutdown or just that the peer closed the 
    *   connection. Call wolfSSL_get_error() for the specific error code.
    *   
    *   SSL_FATAL_ERROR will be returned upon failure when either an error 
    *   occurred or, when using non-blocking sockets, the 
    *   SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE error was received and 
    *   the application needs to call wolfSSL_read() again. 
    *   
    *   Use wolfSSL_get_error() to get a specific error code.
    *   
    *  Parameters
    *  
    *   ssl    pointer to the SSL session, created with wolfSSL_new().
    *   data    buffer where wolfSSL_read() will place data read.
    *   sz    number of bytes to read into data.
    *   
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /* even though the result should be a zero-terminated string, 
         * we'll clear the receive buffer */
        memset(buff, 0, BUFF_SIZE);
        
        /* read the response data from our secure connection */
        if (wolfSSL_read(ssl, buff, BUFF_SIZE - 1) > 0) {
            
            /* one or more bytes received is considered success */
            
            /* Print to stdout any data the server sends */
            WOLFSSL_MSG("wolfSSL_read received message:\n");    
            WOLFSSL_MSG(buff);
        }
        else {
            /* get the integer error value */
            int err = 0;
            err = wolfSSL_get_error(ssl, 0);

            /* get the human-readable error string. */
            char err_buff[80] = "\x0";
            wolfSSL_ERR_error_string(err, err_buff);
            WOLFSSL_ERROR_MSG(err_buff);
            
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_read FAILED.\n");
            ret = WOLFSSL_FAILURE;
        }
    }
    

    /* 
    ***************************************************************************
    *    
    *    Cleanup and return 
    *    
    ***************************************************************************
    */
    if (sockfd != SOCKET_INVALID) {
        close(sockfd); /* Close the connection to the server       */
    }
    
    if (ssl) {
        wolfSSL_free(ssl); /* Free the wolfSSL object                  */
    }
    
    if (ctx) {
        wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object          */
    }
    
    wolfSSL_Cleanup(); /* Cleanup the wolfSSL environment          */

    WOLFSSL_LEAVE("tls_smp_client_task", ret);
    WOLFSSL_MSG("tls_smp_client_task done!\n");
    return ret;
}


/** Event handler for Ethernet events */
static void eth_event_handler(void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data) {
    uint8_t mac_addr[6] = { 0 };
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        /*
         * see ESP-IDF 5.0 note at 
         * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/migration-guides/ethernet.html
         * 
         * esp_eth_ioctl() third argument could take int (bool) number as an input in some cases.
         * However, it was not properly documented and, in addition, the number had to be “unnaturally” 
         * type casted to void * datatype to prevent compiler warnings as shown in below example:
         * 
         * esp_eth_ioctl(eth_handle, ETH_CMD_S_FLOW_CTRL, (void *)true);
         *
         * This could lead to misuse of the esp_eth_ioctl(). Therefore, ESP-IDF 5.0 unified usage of 
         * esp_eth_ioctl(). Its third argument now always acts as pointer to a memory location of specific 
         * type from/to where the configuration option is read/stored.
         * 
         * TODO Migrate Ethernet Drivers to ESP-IDF 5.0
         
        eth_duplex_t new_duplex_mode = ETH_DUPLEX_HALF;
        esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &new_duplex_mode);
        
         */
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);

        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG,
            "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0],
            mac_addr[1],
            mac_addr[2],
            mac_addr[3],
            mac_addr[4],
            mac_addr[5]);
        break;
        
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
        
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
        
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
        
    default:
        ESP_LOGI(TAG, "Other");
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

int set_time() {
    /* we'll also return a result code of zero */
    int res = 0;
    
    //*ideally, we'd like to set time from network, but let's set a default time, just in case */
    struct tm timeinfo;
    timeinfo.tm_year = 2022 - 1900;
    timeinfo.tm_mon = 3;
    timeinfo.tm_mday = 15;
    timeinfo.tm_hour = 8;
    timeinfo.tm_min = 03;
    timeinfo.tm_sec = 10;
    time_t t;
    t = mktime(&timeinfo);

    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);   

    /* set timezone */
    setenv("TZ", TIME_ZONE, 1);
    tzset();

    /* next, let's setup NTP time servers 
     * 
     * see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#sntp-time-synchronization
    */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
    int i = 0;
    for (i = 0; i < NTP_SERVER_COUNT; i++) {
        const char* thisServer = ntpServerList[i];
        if (strncmp(thisServer, "\x00", 1)) {
            /* just in case we run out of NTP servers */
            break;
        }
        sntp_setservername(i, thisServer);
    }
    sntp_init();
    return res;
}


int init_ENC28J60() {
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);


    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_ENC28J60_MISO_GPIO,
        .mosi_io_num = CONFIG_EXAMPLE_ENC28J60_MOSI_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ENC28J60_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_EXAMPLE_ENC28J60_SPI_HOST, &buscfg, 1));
    /* ENC28J60 ethernet driver is based on spi driver */
    spi_device_interface_config_t devcfg = {
        .command_bits = 3,
        .address_bits = 5,
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ENC28J60_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_EXAMPLE_ENC28J60_CS_GPIO,
        .queue_size = 20
    };
    spi_device_handle_t spi_handle = NULL;
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_EXAMPLE_ENC28J60_SPI_HOST, &devcfg, &spi_handle));

    eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(spi_handle);
    enc28j60_config.int_gpio_num = CONFIG_EXAMPLE_ENC28J60_INT_GPIO;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = -1; // ENC28J60 doesn't have SMI interface
    mac_config.smi_mdio_gpio_num = -1;
    esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
    phy_config.reset_gpio_num = -1; // ENC28J60 doesn't have a pin to reset internal PHY
    esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

       
    mac->set_addr(mac, myMacAddress);


    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    /* Register user defined event handers 
     * "ensure that they register the user event handlers as the last thing prior to starting the Ethernet driver." 
    */
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle)); 
    
    return 0;
}

void app_main(void) {
    init_ENC28J60();
    
    // one of the most important aspects of security is the time and date values
    set_time();
    
    for (;;) {
        ESP_LOGI(TAG, "main loop");
        vTaskDelay(DelayTicks ? DelayTicks : 1); /* Minimum delay = 1 tick */     
        tls_smp_client_task();
    }
}

void app_main2(void) {
    ESP_LOGI(TAG, "Start app_main...");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Initialize wifi");
#if (ESP_IDF_VERSION_MAJOR >= 4 && ESP_IDF_VERSION_MINOR >= 1) || \
    (ESP_IDF_VERSION_MAJOR > 5)
    esp_netif_init();
#else
    tcpip_adapter_init();
#endif

    /* */
#if ESP_IDF_VERSION_MAJOR >= 4
    //(void) wifi_event_handler;
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
    * Read "Establishing Wi-Fi or Ethernet Connection" section in
    * examples/protocols/README.md for more information about this function.
    */
    // ESP_ERROR_CHECK(example_connect());
#else
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
        .ssid = TLS_SMP_WIFI_SSID,
        .password = TLS_SMP_WIFI_PASS,
    },
    };
    /* WiFi station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    /* Wifi Set the configuration of the ESP32 STA or AP */ 
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    /* Start Wifi */
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG,
        "connect to ap SSID:%s password:%s",
        TLS_SMP_WIFI_SSID,
        TLS_SMP_WIFI_PASS);
#endif
    ESP_LOGI(TAG, "Set dummy time...");
    //set_time();
}

