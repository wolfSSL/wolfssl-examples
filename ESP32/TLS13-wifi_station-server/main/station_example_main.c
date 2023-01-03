/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* socket includes */
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include  <lwip/apps/sntp.h>

/* wolfSSL */
#define WOLFSSL_ESPIDF
#define WOLFSSL_ESPWROOM32
#define WOLFSSL_USER_SETTINGS


#include <wolfssl/version.h>
/* make sure user_settings.h appears before any other wolfSSL headers */
#include <user_settings.h>
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

/* when using a private config with plain text passwords,
** define USE_MY_PRIVATE_CONFIG.
** note my_private_config.h should be excluded from git updates
*/
#define  USE_MY_PRIVATE_CONFIG

#ifdef  USE_MY_PRIVATE_CONFIG
    #include "/mnt/c/workspace/my_private_config.h"
#else
    /* The examples use WiFi configuration that you can set via project
     *  configuration menu
     *
     * If you'd rather not, just change the below entries to strings with
     *  the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
     */
    #define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
    #define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#endif

/* ESP lwip */
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#define DEFAULT_PORT                     11111

#define TLS_SMP_CLIENT_TASK_NAME         "tls_server_example"
#define TLS_SMP_CLIENT_TASK_WORDS        10240
#define TLS_SMP_CLIENT_TASK_PRIORITY     8

/* include certificates. Note that there is an expiration date!
**
** See also https://github.com/wolfSSL/wolfssl/blob/master/wolfssl/certs_test.h
**
** for example:
**
**  #define USE_CERT_BUFFERS_2048
**  #include <wolfssl/certs_test.h>
*/
#include "embedded_CLIENT_CERT_FILE.h"
#include "embedded_SERVER_CERT_FILE.h"
#include "embedded_SERVER_KEY_FILE.h"

const int NTP_SERVER_COUNT = 3;
const char* ntpServerList[] = {
    "pool.ntp.org",
    "time.nist.gov",
    "utcnist.colorado.edu"
};
const char * TIME_ZONE = "PST-8";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

TickType_t DelayTicks = 5000 / portTICK_PERIOD_MS;
/**
 ******************************************************************************
 ******************************************************************************
 ** USER SETTINGS END
 ******************************************************************************
 ******************************************************************************
 **/

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
** but we only care about two events:
** - we are connected to the AP with an IP
** - we failed to connect after the maximum amount of retries
*/
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT
             &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {

        /* we have WiFi disconnected event, so try reconnect */
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {

        /* we have an IP address! */
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else {
        ESP_LOGI(TAG, "WiFi event not handled: event_id = %li", event_id);
    }
}

int set_time() {
    /* we'll also return a result code of zero */
    int res = 0;

    /*ideally, we'd like to set time from network,
      but let's set a default time, just in case */
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

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security
             * modes including WEP/WPA.However these modes are deprecated and
             * not advisable to be used. In case your Access point doesn't
             * support WPA2, these mode can be enabled by commenting below line
             */
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
    ** or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
    ** The bits are set by event_handler() (see above)
    */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned,
    ** hence we can test which event actually happened.
    */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,
                                                          IP_EVENT_STA_GOT_IP,
                                                          instance_got_ip)
                                                         );
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                                                          ESP_EVENT_ANY_ID,
                                                          instance_any_id)
                                                         );
    vEventGroupDelete(s_wifi_event_group);
}

int tls_smp_server_task() {
    int ret = WOLFSSL_SUCCESS; /* assume success until proven wrong */
    int sockfd = 0; /* the socket that will carry our secure connection */
    struct sockaddr_in servAddr;
    const int BUFF_SIZE = 256;
    char buff[BUFF_SIZE];
    size_t len; /* we'll be looking at the length of messages sent & received */

    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    const char*        reply = "I hear ya fa shizzle!\n";
    int                on;

    static int mConnd = SOCKET_INVALID;
    static int mShutdown = 0;

    /* declare wolfSSL objects */
    WOLFSSL_CTX *ctx = NULL; /* the wolfSSL context object*/


    WOLFSSL *ssl = NULL; /* although called "ssl" this is the secure object
                            for reading and writing data*/

#ifdef HAVE_SIGNAL
    signal(SIGINT, sig_handler);
#endif

#ifdef DEBUG_WOLFSSL
    wolfSSL_Debugging_ON();
    WOLFSSL_MSG("Debug ON v0.2b");
    /* ShowCiphers(); */
#endif /* DEBUG_WOLFSSL */

#ifndef WOLFSSL_TLS13
    ret = WOLFSSL_FAILURE;
    WOLFSSL_ERROR_MSG("ERROR: Example requires TLS v1.3.\n");
#endif /* WOLFSSL_TLS13 */

    /* Initialize the server address struct with zeros */
    memset(&servAddr, 0, sizeof(servAddr));

    /* Fill in the server address */
    servAddr.sin_family      = AF_INET; /* using IPv4      */
    servAddr.sin_port        = htons(DEFAULT_PORT); /* on DEFAULT_PORT */
    servAddr.sin_addr.s_addr = INADDR_ANY; /* from anywhere   */

    /*
    ***************************************************************************
    * Create a socket that uses an Internet IPv4 address,
    * Sets the socket to be stream based (TCP),
    * 0 means choose the default protocol.
    *
    *  #include <sys/socket.h>
    *
    *  int socket(int domain, int type, int protocol);
    *
    *  The socket() function shall create an unbound socket in a communications
    *  domain, and return a file descriptor that can be used in later function
    *  calls that operate on sockets.
    *
    *  The socket() function takes the following arguments:
    *    domain     Specifies the communications domain in which a
    *                 socket is to be created.
    *    type       Specifies the type of socket to be created.
    *    protocol   Specifies a particular protocol to be used with the socket.
    *               Specifying a protocol of 0 causes socket() to use an
    *               unspecified default protocol appropriate for the
    *               requested socket type.
    *
    *    The domain argument specifies the address family used in the
    *    communications domain. The address families supported by the system
    *    are implementation-defined.
    *
    *    Symbolic constants that can be used for the domain argument are
    *    defined in the <sys/socket.h> header.
    *
    *  The type argument specifies the socket type, which determines the
    *  semantics of communication over the socket. The following socket types
    *  are defined; implementations may specify additional socket types:
    *
    *    SOCK_STREAM    Provides sequenced, reliable, bidirectional,
    *                   connection-mode byte streams, and may provide a
    *                   transmission mechanism for out-of-band data.
    *    SOCK_DGRAM     Provides datagrams, which are connectionless-mode,
    *                   unreliable messages of fixed maximum length.
    *    SOCK_SEQPACKET Provides sequenced, reliable, bidirectional,
    *                   connection-mode transmission paths for records.
    *                   A record can be sent using one or more output
    *                   operations and received using one or more input
    *                   operations, but a single operation never transfers
    *                   part of more than one record. Record boundaries
    *                   are visible to the receiver via the MSG_EOR flag.
    *
    *                   If the protocol argument is non-zero, it shall
    *                   specify a protocol that is supported by the address
    *                   family. If the protocol argument is zero, the default
    *                   protocol for this address family and type shall be
    *                   used. The protocols supported by the system are
    *                   implementation-defined.
    *
    *    The process may need to have appropriate privileges to use the
    *    socket() function or to create some sockets.
    *
    *  Return Value
    *    Upon successful completion, socket() shall return a non-negative
    *    integer, the socket file descriptor.
    *
    *    Otherwise, a value of -1 shall be returned
    *    and errno set to indicate the error.
    *
    *  Errors; The socket() function shall fail if:
    *
    *    EAFNOSUPPORT    The implementation does not support the specified
    *                    address family.
    *    EMFILE          No more file descriptors are available
    *                    for this process.
    *    ENFILE          No more file descriptors are available for the system.
    *    EPROTONOSUPPORT The protocol is not supported by the address family,
    *                    or the protocol is not supported by the implementation.
    *    EPROTOTYPE      The socket type is not supported by the protocol.
    *
    *  The socket() function may fail if:
    *
    *    EACCES  The process does not have appropriate privileges.
    *    ENOBUFS Insufficient resources were available in the system to
    *            perform the operation.
    *    ENOMEM  Insufficient memory was available to fulfill the request.
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
        WOLFSSL_ERROR_MSG("Skipping socket create.\n");
    }


    /*
    ***************************************************************************
    * set SO_REUSEADDR on socket
    *
    *  #include <sys/types.h>
    *  # include <sys / socket.h>
    *  int getsockopt(int sockfd,
    *    int level,
    *    int optname,
    *    void *optval,
    *    socklen_t *optlen); int setsockopt(int sockfd,
    *    int level,
    *    int optname,
    *    const void *optval,
    *    socklen_t optlen);
    *
    *  setsockopt() manipulates options for the socket referred to by the file
    *  descriptor sockfd. Options may exist at multiple protocol levels; they
    *  are always present at the uppermost socket level.
    *
    *  When manipulating socket options, the level at which the option resides
    *  and the name of the option must be specified. To manipulate options at
    *  the sockets API level, level is specified as SOL_SOCKET. To manipulate
    *  options at any other level the protocol number of the appropriate
    *  protocol controlling the option is supplied. For example, to indicate
    *  that an option is to be interpreted by the TCP protocol, level should
    *  be set to the protocol number of TCP
    *
    *  Return Value
    *    On success, zero is returned.
    *    On error, -1 is returned, and errno is set appropriately.
    *
    *  Errors
    *    EBADF       The argument sockfd is not a valid descriptor.
    *    EFAULT      The address pointed to by optval is not in a valid part of
    *                the process address space. For getsockopt(), this error
    *                may also be returned if optlen is not in a valid part of
    *                the process address space.
    *    EINVAL      optlen invalid in setsockopt(). In some cases this error
    *                can also occur for an invalid value in optval
    *                e.g., for the IP_ADD_MEMBERSHIP option described in ip(7)
    *    ENOPROTOOPT The option is unknown at the level indicated.
    *    ENOTSOCK    The argument sockfd is a file, not a socket.
    *
    *  see: https://linux.die.net/man/2/setsockopt
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /* make sure server is setup for reuse addr/port */
        on = 1;
        int soc_ret = setsockopt(sockfd,
            SOL_SOCKET,
            SO_REUSEADDR,
            (char*)&on,
            (socklen_t)sizeof(on));

        if (soc_ret == 0) {
            WOLFSSL_MSG("setsockopt re-use addr successful\n");
        }
        else {
            ESP_LOGE(TAG, "setsockopt failed with code %i", soc_ret);
            ret = WOLFSSL_FAILURE;
            WOLFSSL_ERROR_MSG("ERROR: failed to setsockopt addr on socket.\n");
        }
    }
    else {
        WOLFSSL_ERROR_MSG("Skipping setsockopt addr\n");
    }

#ifdef SO_REUSEPORT
    /* see above for details on getsockopt  */
    if (ret == WOLFSSL_SUCCESS) {
        int soc_ret = setsockopt(sockfd,
            SOL_SOCKET,
            SO_REUSEPORT,
            (char*)&on,
            (socklen_t)sizeof(on));

        if (soc_ret == 0) {
            WOLFSSL_MSG("setsockopt re-use port successful\n");
        }
        else  {
            WOLFSSL_MSG("Note: failed to setsockopt port on socket."
                        " >> IGNORED << \n");
        }
    }
    else {
        WOLFSSL_ERROR_MSG("Skipping setsockopt port\n");
    }
#else
    WOLFSSL_MSG("SO_REUSEPORT not configured for setsockopt to re-use port\n");
#endif

    /*
    ***************************************************************************
    *  #include <sys/types.h>
    *  #include <sys/socket.h>
    *
    *  int bind(int sockfd,
    *      const struct sockaddr *addr,
    *      socklen_t addrlen);
    *
    *  Description
    *
    *  When a socket is created with socket(2), it exists in a name
    *  space(address family) but has no address assigned to it.
    *
    *  bind() assigns the address specified by addr to the socket referred to
    *  by the file descriptor sockfd.addrlen specifies the size, in bytes, of
    *  the address structure pointed to by addr.Traditionally, this operation
    *  is called "assigning a name to a socket".
    *
    *   It is normally necessary to assign a local address using bind() before
    *   a SOCK_STREAM socket may receive connections.
    *
    *  Return Value
    *    On success, zero is returned.
    *    On error, -1 is returned, and errno is set appropriately.
    *
    *  Errors
    *    EACCES     The address is protected, and
    *               the user is not the superuser.
    *    EADDRINUSE The given address is already in use.
    *    EBADF      sockfd is not a valid descriptor.
    *    EINVAL     The socket is already bound to an address.
    *    ENOTSOCK   sockfd is a descriptor for a file, not a socket.
    *
    *   see: https://linux.die.net/man/2/bind
    *
    *       https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/lwip.html
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        /* Bind the server socket to our port */
        int soc_ret = bind(sockfd,
                           (struct sockaddr*)&servAddr,
                           sizeof(servAddr)
                          );

        if (soc_ret > -1) {
            WOLFSSL_MSG("socket bind successful\n");
        }
        else {
            ret = WOLFSSL_FAILURE;
            WOLFSSL_ERROR_MSG("ERROR: failed to bind to socket.\n");
        }
    }

    /*
    ***************************************************************************
    *  Listen for a new connection, allow 5 pending connections
    *
    *  #include <sys/types.h>
    *  #include <sys/socket.h>
    *  int listen(int sockfd, int backlog);
    *
    *  Description
    *
    *  listen() marks the socket referred to by sockfd as a passive socket,
    *  that is, as a socket that will be used to accept incoming connection
    *  requests using accept.
    *
    *  The sockfd argument is a file descriptor that refers to a socket of
    *  type SOCK_STREAM or SOCK_SEQPACKET.
    *
    *  The backlog argument defines the maximum length to which the queue of
    *  pending connections for sockfd may grow.If a connection request arrives
    *  when the queue is full, the client may receive an error with an
    *  indication of ECONNREFUSED or, if the underlying protocol supports
    *  retransmission, the request may be ignored so that a later reattempt
    *  at connection succeeds.
    *
    *   Return Value
    *     On success, zero is returned.
    *     On Error, -1 is returned, and errno is set appropriately.
    *   Errors
    *     EADDRINUSE   Another socket is already listening on the same port.
    *     EBADF        The argument sockfd is not a valid descriptor.
    *     ENOTSOCK     The argument sockfd is not a socket.
    *     EOPNOTSUPP   The socket is not of a type that supports
    *                  the listen() operation.
    *
    *  see: https://linux.die.net/man/2/listen
    */

    if (ret == WOLFSSL_SUCCESS) {
        int soc_ret = listen(sockfd, 5);
        if (soc_ret > -1) {
            WOLFSSL_MSG("socket listen successful\n");
        }
        else {
            ret = WOLFSSL_FAILURE;
            WOLFSSL_ERROR_MSG("ERROR: failed to listen to socket.\n");
        }
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
    *    pointer If successful the call will return a pointer
    *    to the newly-created WOLFSSL_CTX.

    *    NULL upon failure.
    *
    *  Parameters
    *    method pointer to the desired WOLFSSL_METHOD to use for SSL context.
    *    This is created using one of the wolfSSLvXX_XXXX_method() functions to
    *    specify SSL/TLS/DTLS protocol level.
    *
    *  see https://www.wolfssl.com/doxygen/group__Setup.html#gadfa552e771944a6a1102aa43f45378b5
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        WOLFSSL_METHOD* method = wolfTLSv1_3_server_method();
        WOLFSSL_MSG("calling wolfTLSv1_3_server_method");
        if (method == NULL) {
            WOLFSSL_ERROR_MSG("ERROR : "
                              "failed to get wolfTLSv1_3_server_method.\n");
            ret = WOLFSSL_FAILURE;
        }
        else {
            WOLFSSL_MSG("calling wolfSSL_CTX_new");
            ctx = wolfSSL_CTX_new(method);

            if (ctx == NULL) {
                WOLFSSL_ERROR_MSG("ERROR : failed to create WOLFSSL_CTX\n");
                ret = WOLFSSL_FAILURE;
            }
        }
    }
    else {
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
    *    SSL_BAD_FILE     will be returned if the file doesn’t exist,
    *                     can’t be read, or is corrupted.
    *    MEMORY_E         will be returned if an out of memory condition occurs.
    *    ASN_INPUT_E      will be returned if Base16 decoding fails on the file.
    *
    *  Parameters
    *    ssl    pointer to the SSL session, created with wolfSSL_new().
    *    in     buffer containing certificate to load.
    *    sz     size of the certificate located in buffer.
    *    format format of the certificate to be loaded.
    *           Possible values are SSL_FILETYPE_ASN1 or SSL_FILETYPE_PEM.
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
            WOLFSSL_ERROR_MSG("ERROR: "
                              "wolfSSL_CTX_use_certificate_buffer failed\n");
        }
    }
    else {
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
    *    SSL_SUCCESS      upon success.
    *    SSL_BAD_FILETYPE returned if the file is the wrong format.
    *    SSL_BAD_FILE     returned if the file doesn’t exist,
    *                     can’t be read, or is corrupted.
    *
    *    MEMORY_E         returned if an out of memory condition occurs.
    *    ASN_INPUT_E      returned if Base16 decoding fails on the file.
    *    NO_PASSWORD      returned if the key file is encrypted
    *                     but no password is provided.
    *
    *  Parameters
    *    ctx     pointer to the SSL context, created with wolfSSL_CTX_new().
    *            in the input buffer containing the private key to be loaded.
    *
    *    sz      the size of the input buffer.
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
            WOLFSSL_ERROR_MSG("ERROR: "
                              "wolfSSL_CTX_use_PrivateKey_buffer failed\n");
        }
    }
    else {
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
    *    SSL_SUCCESS       upon success
    *    SSL_BAD_FILETYPE  returned if the file is the wrong format.
    *    SSL_BAD_FILE      returned if the file doesn’t exist, can’t be read,
    *                      or is corrupted.
    *    MEMORY_E          returned if an out of memory condition occurs.
    *    ASN_INPUT_E       returned if Base16 decoding fails on the file.
    *    BUFFER_E          returned if a chain buffer is bigger than
    *                      the receiving buffer.
    *
    *  Parameters
    *
    *    ctx     pointer to the SSL context, created with wolfSSL_CTX_new().
    *    in      pointer to the CA certificate buffer.
    *    sz      size of the input CA certificate buffer, in.
    *    format  format of the buffer certificate, either SSL_FILETYPE_ASN1
    *            or SSL_FILETYPE_PEM.
    *
    * see https://www.wolfssl.com/doxygen/group__CertsKeys.html#gaa37539cce3388c628ac4672cf5606785
    ***************************************************************************
    */
    if (ret == WOLFSSL_SUCCESS) {
        ret = wolfSSL_CTX_load_verify_buffer(ctx,
                                             CA_FILE,
                                             sizeof_CA_FILE(),
                                             WOLFSSL_FILETYPE_PEM
                                            );

        if (ret == WOLFSSL_SUCCESS) {
            WOLFSSL_MSG("wolfSSL_CTX_load_verify_buffer successful\n");
        }
        else {
            WOLFSSL_ERROR_MSG("ERROR: "
                              "wolfSSL_CTX_load_verify_buffer failed\n");
        }
    }
    else {
        WOLFSSL_ERROR_MSG("skipping wolfSSL_CTX_load_verify_buffer\n");
    }

    /* Require mutual authentication */
    wolfSSL_CTX_set_verify(ctx,
        WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT,
        NULL);

    /* Continue to accept clients until mShutdown is issued */
    while (!mShutdown && (ret == WOLFSSL_SUCCESS)) {
        WOLFSSL_MSG("Waiting for a connection...\n");

        /* Accept client connections */
        if ((mConnd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
            == -1) {

            ret = -1;
            goto exit;
            WOLFSSL_ERROR_MSG("ERROR: failed socket connection accept\n");
            ret = WOLFSSL_FAILURE;
        }

        /* Create a WOLFSSL object */
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            WOLFSSL_ERROR_MSG("ERROR: failed to create WOLFSSL object\n");
            ret = -1;
            goto exit;
            WOLFSSL_ERROR_MSG("ERROR: failed wolfSSL_new during loop\n");
            ret = WOLFSSL_FAILURE;
        }

        /* Attach wolfSSL to the socket */
        wolfSSL_set_fd(ssl, mConnd);

#ifdef HAVE_SECRET_CALLBACK
        /* required for getting random used */
        wolfSSL_KeepArrays(ssl);

        /* optional logging for wireshark */
        wolfSSL_set_tls13_secret_cb(ssl,
            Tls13SecretCallback,
            (void*)WOLFSSL_SSLKEYLOGFILE_OUTPUT);
#endif

        /* Establish TLS connection */
        if ((ret = wolfSSL_accept(ssl)) != WOLFSSL_SUCCESS) {
            WOLFSSL_ERROR_MSG("ERROR: wolfSSL_accept\n");

            ret = WOLFSSL_FAILURE;
            ESP_LOGE(TAG, "wolfSSL_accept error = %d\n",
                           wolfSSL_get_error(ssl, ret));
            goto exit;
        }
        else {
            WOLFSSL_MSG("Client connected successfully\n");
        }

#ifdef HAVE_SECRET_CALLBACK
        wolfSSL_FreeArrays(ssl);
#endif

        /* Read the client data into our buff array */
        memset(buff, 0, sizeof(buff));
        if ((ret = wolfSSL_read(ssl, buff, sizeof(buff) - 1)) < 0) {
            ESP_LOGE(TAG, "wolfSSL_read error = %d\n",
                           wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        /* Print any data the client sends */
        ESP_LOGI(TAG, "Client: %s\n", buff);

        /* Check for server shutdown command */
        if (strncmp(buff, "shutdown", 8) == 0) {
            ESP_LOGI(TAG, "Shutdown command issued!\n");
            mShutdown = 1;
        }

        /* Write our reply into buff */
        memset(buff, 0, sizeof(buff));
        memcpy(buff, reply, strlen(reply));
        len = strnlen(buff, sizeof(buff));

        /* Reply back to the client */
        if ((ret = wolfSSL_write(ssl, buff, len)) != len) {
            ESP_LOGE(TAG, "wolfSSL_write error = %d\n",
                           wolfSSL_get_error(ssl, ret));
            goto exit;
        }

        /* Cleanup after this connection */
        wolfSSL_shutdown(ssl);
        if (ssl) {
            wolfSSL_free(ssl); /* Free the wolfSSL object          */
            ssl = NULL;
        }
        if (mConnd != SOCKET_INVALID) {
            close(mConnd); /* Close the connection to the client   */
            mConnd = SOCKET_INVALID;
        }
    }

    WOLFSSL_MSG("Shutdown complete\n");

    /*
    ***************************************************************************
    *
    *    Cleanup and return
    *
    ***************************************************************************
    */
exit:
    if (mConnd != SOCKET_INVALID) {
        close(mConnd); /* Close the connection to the client      */
        mConnd = SOCKET_INVALID;
    }

    if (sockfd != SOCKET_INVALID) {
        close(sockfd); /* Close the socket listening for clients  */
        sockfd = SOCKET_INVALID;
    }

    if (ssl) {
        wolfSSL_free(ssl); /* Free the wolfSSL object             */
    }


    if (ctx) {
        wolfSSL_CTX_free(ctx); /* Free the wolfSSL context object */
    }

    wolfSSL_Cleanup(); /* Cleanup the wolfSSL environment         */

    return ret;
}


void app_main(void)
{
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "---------------------- BEGIN MAIN ----------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");
    ESP_LOGI(TAG, "--------------------------------------------------------");


    ESP_LOGI(TAG, "LIBWOLFSSL_VERSION_STRING = %s", LIBWOLFSSL_VERSION_STRING);
    ESP_LOGI(TAG, "CONFIG_IDF_TARGET = %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ = %u MHz", CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ);
    ESP_LOGI(TAG, "Xthal_have_ccount = %u", Xthal_have_ccount);

    ESP_LOGI(TAG, "Stack HWM: %d\n", uxTaskGetStackHighWaterMark(NULL));

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    set_time();

    for (;;) {
        ESP_LOGI(TAG, "main loop");
        vTaskDelay(DelayTicks ? DelayTicks : 1); /* Minimum delay = 1 tick */
        tls_smp_server_task();

        /* done. wait forever */
        for (;;)
        {
            vTaskDelay(DelayTicks ? DelayTicks : 1); /* Minimum delay = 1 tick
                                                     */
        }
    }
}
