/*
 * client-dtls13.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
 *
 *=============================================================================
 *
 * Bare-bones example of a DTLS 1.3 client for instructional/learning purposes.
 * This example uses blocking sockets for simplicity.
 *
 * Define USE_DTLS12 to use DTLS 1.2 instead of DTLS 1.3
/*
Tested with:

1) Intel Galileo acting as the Client, with a laptop acting as a server using
   the server example provided in examples/server.
   Legacy Arduino v1.86 was used to compile and program the Galileo

2) Espressif ESP32 WiFi

3) Arduino Due, Nano33 IoT, Nano RP-2040
*/

/*
 * Note to code editors: the Arduino client and server examples are edited in
 * parallel for side-by-side comparison between examples.
 */

/* If you have a private include, define it here, otherwise edit WiFi params */
/* #define MY_PRIVATE_CONFIG "/workspace/my_private_config.h" */

#if defined(ARDUINO) && defined(ESP8266)
    #warning "This example is not yet supported on Arduino ESP8266"
#endif

#if defined(DEBUG_WOLFSSL)
    /* Optionally enabled verbose wolfSSL debugging */
    #define DEBUG_WOLFSSL_MESSAGES_ON
#else
    /* DEBUG_WOLFSSL needs to be enabled */
    #undef DEBUG_WOLFSSL_MESSAGES_ON
#endif

/* set REPEAT_CONNECTION to a non-zero value to continually run the example. */
#define REPEAT_CONNECTION 0

/* Edit this with your other DTLS host server address to connect to: */
#define WOLFSSL_DTLS_SERVER_HOST "192.168.1.107"

/* wolfssl TLS examples communicate on port 11111 */
#define WOLFSSL_PORT 11111

/* Choose a monitor serial baud rate: 9600, 14400, 19200, 57600, 74880, etc. */
#define SERIAL_BAUD 115200

/* We'll wait up to 2000 milliseconds to properly shut down connection */
#define SHUTDOWN_DELAY_MS 2000

/* Number of times to retry connection.  */
#define RECONNECT_ATTEMPTS 20

/* Number of DTLS messages to send. Use -1 for continual messages. */
#define DTLS_MESSAGE_CT 42

/* Assume bad socket until proven otherwise */
#define INVALID_SOCKET -1

/* Maximum size in bytes of buffer to send and receive */
#define MAXLINE   128

/* Optional stress test. Define to consume memory until exhausted: */
/* #define MEMORY_STRESS_TEST */

/* Choose client or server example, not both. */
#define WOLFSSL_CLIENT_EXAMPLE
/* #define WOLFSSL_SERVER_EXAMPLE */

#if defined(MY_PRIVATE_CONFIG)
    /* the /workspace directory may contain a private config
     * excluded from GitHub with items such as WiFi passwords */
    #include MY_PRIVATE_CONFIG
    static const char ssid[]     PROGMEM = MY_ARDUINO_WIFI_SSID;
    static const char password[] PROGMEM = MY_ARDUINO_WIFI_PASSWORD;
#else
    /* when using WiFi capable boards: */
    static const char ssid[]     PROGMEM = "your_SSID";
    static const char password[] PROGMEM = "your_PASSWORD";
#endif

#define BROADCAST_ADDRESS "255.255.255.255"

/* There's an optional 3rd party NTPClient library by Fabrice Weinberg.
 * If it is installed, uncomment define USE_NTP_LIB here: */
/* #define USE_NTP_LIB */
#ifdef USE_NTP_LIB
    #include <NTPClient.h>
#endif

/* wolfSSL user_settings.h must be included from settings.h
 * Make all configurations changes in user_settings.h
 * Do not edit wolfSSL `settings.h` or `config.h` files.
 * Do not explicitly include user_settings.h in any source code.
 * Each Arduino sketch that uses wolfSSL must have: #include "wolfssl.h"
 * C/C++ source files can use: #include <wolfssl/wolfcrypt/settings.h>
 * The wolfSSL "settings.h" must be included in each source file using wolfSSL.
 * The wolfSSL "settings.h" must appear before any other wolfSSL include.
 */
#include <wolfssl.h>
/* Important: make sure settings.h appears before any other wolfSSL headers */
#include <wolfssl/wolfcrypt/settings.h>
/* Reminder: settings.h includes user_settings.h
 * For ALL project wolfSSL settings, see:
 * [your path]/Arduino\libraries\wolfSSL\src\user_settings.h   */
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#ifndef WOLFSSL_DTLS
    /* Support for DTLS by default was added after wolfSSL v5.8.2 release */
    #error "This example requires WOLFSSL_DTLS. See user_settings.h in the Arduino wolfssl library"
#endif

/* Define DEBUG_WOLFSSL in user_settings.h for more verbose logging. */
#if defined(DEBUG_WOLFSSL)
    #define PROGRESS_DOT F("")
#else
    #define PROGRESS_DOT F(".")
#endif

/* Convert a macro to a string */
#define xstr(x) str(x)
#define str(x) #x

/* optional board-specific networking includes */
#if defined(ESP32)
    #define USING_WIFI
    #include <WiFi.h>
    #include <WiFiUdp.h>
    #ifdef USE_NTP_LIB
        WiFiUDP ntpUDP;
    #endif
    /* Ensure the F() flash macro is defined */
    #ifndef F
        #define F
    #endif
    WiFiClient client;

#elif defined(ESP8266)
    #define USING_WIFI
    #include <ESP8266WiFi.h>
    WiFiClient client;

#elif defined(ARDUINO_SAM_DUE)
    #include <SPI.h>
    /* There's no WiFi/Ethernet on the Due. Requires Ethernet Shield.
    /* Needs "Ethernet by Various" library to be installed. Tested with V2.0.2 */
    #include <Ethernet.h>
    EthernetClient client;
#elif defined(ARDUINO_AVR_ETHERNET) || defined(ARDUINO_AVR_LEONARDO_ETH)
    /* Boards such as arduino:avr:ethernet and arduino:avr:leonardoeth */
    #include <Ethernet.h>
    EthernetClient client;

#elif defined(ARDUINO_SAMD_NANO_33_IOT)
    #define USING_WIFI
    #include <SPI.h>
    #include <WiFiNINA.h> /* Needs Arduino WiFiNINA library installed manually */
    WiFiClient client;

#elif defined(ARDUINO_ARCH_RP2040)
    #define USING_WIFI
    #include <SPI.h>
    #include <WiFiNINA.h>
    WiFiClient client;

#elif defined(ARDUINO_SAMD_TIAN)
    #include <Bridge.h>
    #include <HttpClient.h>
    HttpClient client;
    /*  Arduino Tian does not support network shields like the standard Ethernet or Wi-Fi shields. */
    #error "HttpClient cannot be used for this example"
#elif defined(ARDUINO_PORTENTA_X8)
    /* The Portenta is a Linux device. See wolfSSL examples:
     * https://github.com/wolfSSL/wolfssl/tree/master/examples
     * By default Serial is disabled and mapped to ErrorSerial */
    #include <SerialRPC.h>

    /* ----No - network placeholders(compile - only) ---- */
    #include <IPAddress.h>
    struct X8NoNetClient {
        int write(const uint8_t*, size_t) { return -1; }
        int available() { return 0; }
        int read() { return -1; }
        void stop() {}
        bool connected() { return false; }
        IPAddress remoteIP() { return IPAddress(0, 0, 0, 0); }
    };
    struct X8NoNetServer {
        explicit X8NoNetServer(uint16_t) {}
        void begin() {}
        X8NoNetClient available() { return X8NoNetClient(); }
    };

    X8NoNetClient client;
    X8NoNetServer server(WOLFSSL_PORT);
#elif defined(USING_WIFI)
    #define USING_WIFI
    #include <WiFi.h>
    #include <WiFiUdp.h>
    #ifdef USE_NTP_LIB
        WiFiUDP ntpUDP;
    #endif
    WiFiClient client;

/* TODO
#elif defined(OTHER_BOARD)
*/
#else
    /* assume all other boards using WiFi library. Edit as needed: */
    #include <WiFi.h>
    #define USING_WIFI
    WiFiClient client;
#endif

/* Only for syntax highlighters to show interesting options enabled: */
#if defined(HAVE_SNI)                           \
   || defined(HAVE_MAX_FRAGMENT)                  \
   || defined(HAVE_TRUSTED_CA)                    \
   || defined(HAVE_TRUNCATED_HMAC)                \
   || defined(HAVE_CERTIFICATE_STATUS_REQUEST)    \
   || defined(HAVE_CERTIFICATE_STATUS_REQUEST_V2) \
   || defined(HAVE_SUPPORTED_CURVES)              \
   || defined(HAVE_ALPN)                          \
   || defined(HAVE_SESSION_TICKET)                \
   || defined(HAVE_SECURE_RENEGOTIATION)          \
   || defined(HAVE_SERVER_RENEGOTIATION_INFO)
#endif

static const char host[] PROGMEM = WOLFSSL_DTLS_SERVER_HOST; /* server to connect to */
static const int port PROGMEM = WOLFSSL_PORT; /* port on server to connect to */

static WOLFSSL_CTX* ctx = NULL;
static WOLFSSL* ssl = NULL;
static char* wc_error_message = (char*)malloc(80 + 1);
static char errBuf[80];

#if defined(MEMORY_STRESS_TEST)
    #define MEMORY_STRESS_ITERATIONS 100
    #define MEMORY_STRESS_BLOCK_SIZE 1024
    #define MEMORY_STRESS_INITIAL (4*1024)
    static char* memory_stress[MEMORY_STRESS_ITERATIONS]; /* typically 1K per item */
    static int mem_ctr        = 0;
#endif

static int EthernetSend(WOLFSSL* ssl, char* msg, int sz, void* ctx);
static int EthernetReceive(WOLFSSL* ssl, char* reply, int sz, void* ctx);
static int reconnect = RECONNECT_ATTEMPTS;
#if 0
/* optional showPeerEx, currently disabled  */
static int lng_index PROGMEM = 0; /* 0 = English */
#endif

#if defined(__arm__)
    #include <malloc.h>
    extern char _end;
    extern "C" char *sbrk(int i);
    static char *ramstart=(char *)0x20070000;
    static char *ramend=(char *)0x20088000;
#endif

/*****************************************************************************/
/* fail_wait - in case of unrecoverable error                                */
/*****************************************************************************/
int fail_wait(void) {
    show_memory();

    Serial.println(F("Failed. Halt."));
    while (1) {
        delay(1000);
    }
    return 0;
}

/*****************************************************************************/
/* show_memory() to optionally view during debugging.                         */
/*****************************************************************************/
int show_memory(void)
{
#if defined(__arm__)
    struct mallinfo mi = mallinfo();

    char *heapend=sbrk(0);
    register char * stack_ptr asm("sp");
    #if defined(DEBUG_WOLFSSL_VERBOSE)
        Serial.print("    arena=");
        Serial.println(mi.arena);
        Serial.print("  ordblks=");
        Serial.println(mi.ordblks);
        Serial.print(" uordblks=");
        Serial.println(mi.uordblks);
        Serial.print(" fordblks=");
        Serial.println(mi.fordblks);
        Serial.print(" keepcost=");
        Serial.println(mi.keepcost);
    #endif

    #if defined(DEBUG_WOLFSSL) || defined(MEMORY_STRESS_TEST)
        Serial.print("Estimated free memory: ");
        Serial.print(stack_ptr - heapend + mi.fordblks);
        Serial.println(F(" bytes"));
    #endif

    #if (0)
        /* Experimental: not supported on all devices: */
        Serial.print("RAM Start %lx\n", (unsigned long)ramstart);
        Serial.print("Data/Bss end %lx\n", (unsigned long)&_end);
        Serial.print("Heap End %lx\n", (unsigned long)heapend);
        Serial.print("Stack Ptr %lx\n",(unsigned long)stack_ptr);
        Serial.print("RAM End %lx\n", (unsigned long)ramend);

        Serial.print("Heap RAM Used: ",mi.uordblks);
        Serial.print("Program RAM Used ",&_end - ramstart);
        Serial.print("Stack RAM Used ",ramend - stack_ptr);

        Serial.print("Estimated Free RAM: %d\n\n",stack_ptr - heapend + mi.fordblks);
    #endif
#else
    Serial.println(F("show_memory() not implemented for this platform"));
#endif
    return 0;
}

/*****************************************************************************/
/* Arduino setup_hardware()                                                  */
/*****************************************************************************/
int setup_hardware(void) {
    int ret = 0;

#if defined(ARDUINO_SAMD_NANO_33_IOT)
    Serial.println(F("Detected known tested and working Arduino Nano 33 IoT"));
#elif defined(ARDUINO_ARCH_RP2040)
    Serial.println(F("Detected known tested and working Arduino RP-2040"));
#elif defined(__arm__) && defined(ID_TRNG) && defined(TRNG)
    /* need to manually turn on random number generator on Arduino Due, etc. */
    pmc_enable_periph_clk(ID_TRNG);
    trng_enable(TRNG);
    Serial.println(F("Enabled ARM TRNG"));
#endif

    show_memory();
    randomSeed(analogRead(0));
    return ret;
}

/*****************************************************************************/
/* Arduino setup_datetime()                                                  */
/*   The device needs to have a valid date within the valid range of certs.  */
/*****************************************************************************/
int setup_datetime(void) {
    int ret = 0;
    int ntp_tries = 20;

    /* we need a date in the range of cert expiration */
#ifdef USE_NTP_LIB
    #if defined(ESP32)
        NTPClient timeClient(ntpUDP, "pool.ntp.org");

        timeClient.begin();
        timeClient.update();
        delay(1000);
        while (!timeClient.isTimeSet() && (ntp_tries > 0)) {
            timeClient.forceUpdate();
            Serial.println(F("Waiting for NTP update"));
            delay(2000);
            ntp_tries--;
        }
        if (ntp_tries <= 0) {
            Serial.println(F("Warning: gave up waiting on NTP"));
        }
        Serial.println(timeClient.getFormattedTime());
        Serial.println(timeClient.getEpochTime());
    #endif
#endif

#if defined(ESP32)
    /* see esp32-hal-time.c */
    ntp_tries = 5;
    /* Replace "pool.ntp.org" with your preferred NTP server */
    configTime(0, 0, "pool.ntp.org");

    /* Wait for time to be set */
    while ((time(nullptr) <= 100000) && ntp_tries > 0) {
        Serial.println(F("Waiting for time to be set..."));
        delay(2000);
        ntp_tries--;
    }
#endif

    return ret;
} /* setup_datetime */

/*****************************************************************************/
/* Arduino setup_network()                                                   */
/*****************************************************************************/
int setup_network(void) {
    int ret = 0;

#if defined(USING_WIFI)
    int status = WL_IDLE_STATUS;

    /* The ESP8266 & ESP32 support both AP and STA. We'll use STA: */
    #if defined(ESP8266) || defined(ESP32)
        WiFi.mode(WIFI_STA);
    #else
        String fv;
        if (WiFi.status() == WL_NO_MODULE) {
            Serial.println("Communication with WiFi module failed!");
            /* don't continue if no network */
            while (true) ;
        }

        fv = WiFi.firmwareVersion();
        if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
            Serial.println("Please upgrade the firmware");
        }
    #endif

    Serial.print(F("Connecting to WiFi "));
    Serial.print(ssid);
    status = WiFi.begin(ssid, password);
    while (status != WL_CONNECTED) {
        delay(1000);
        Serial.print(F("."));
        Serial.print(status);
        status = WiFi.status();
    }

    Serial.println(F(" Connected!"));
#else
    /* Newer Ethernet shields have a
     * MAC address printed on a sticker on the shield */
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    IPAddress ip(192, 168, 1, 42);
    IPAddress myDns(192, 168, 1, 1);
    Ethernet.init(10); /* Most Arduino shields */
    /* Ethernet.init(5);   * MKR ETH Shield */
    /* Ethernet.init(0);   * Teensy 2.0 */
    /* Ethernet.init(20);  * Teensy++ 2.0 */
    /* Ethernet.init(15);  * ESP8266 with Adafruit FeatherWing Ethernet */
    /* Ethernet.init(33);  * ESP32 with Adafruit FeatherWing Ethernet */
    Serial.println(F("Initialize Ethernet with DHCP:"));
    if (Ethernet.begin(mac) == 0) {
        Serial.println(F("Failed to configure Ethernet using DHCP"));
        /* Check for Ethernet hardware present */
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println(F("Ethernet shield was not found."));
            while (true) {
                delay(1); /* do nothing */
            }
        }
        if (Ethernet.linkStatus() == LinkOFF) {
            Serial.println(F("Ethernet cable is not connected."));
        }
        /* try to configure using IP address instead of DHCP : */
        Ethernet.begin(mac, ip, myDns);
    }
    else {
        Serial.print(F("  DHCP assigned IP "));
        Serial.println(Ethernet.localIP());
    }
    /* We'll assume the Ethernet connection is ready to go. */
#endif

    Serial.println(F("********************************************************"));
    Serial.print(F("      wolfSSL Example Client IP = "));
#if defined(USING_WIFI)
    Serial.println(WiFi.localIP());
#else
    Serial.println(Ethernet.localIP());
#endif
    Serial.print(F("   Configured Server Host to connect to: "));
    Serial.println(host);
    Serial.println(F("********************************************************"));
    Serial.println(F("Setup network complete."));

    return ret;
}

/*****************************************************************************/
/* Arduino setup_wolfssl()                                                   */
/*****************************************************************************/
int setup_wolfssl(void)
{
    int ret = 0;
    WOLFSSL_METHOD* method;

    /* Show a revision of wolfssl user_settings.h file in use when available: */
#if defined(WOLFSSL_USER_SETTINGS_ID)
    Serial.print(F("WOLFSSL_USER_SETTINGS_ID: "));
    Serial.println(F(WOLFSSL_USER_SETTINGS_ID));
#else
    Serial.println(F("No WOLFSSL_USER_SETTINGS_ID found."));
#endif

#if defined(NO_WOLFSSL_SERVER)
    Serial.println(F("wolfSSL server code disabled to save space."));
#endif
#if defined(NO_WOLFSSL_CLIENT)
    Serial.println(F("wolfSSL client code disabled to save space."));
#endif

#if defined(DEBUG_WOLFSSL)
    Serial.println(F("wolfSSL Debugging is available! (DEBUG_WOLFSSL)"));
    #if defined(DEBUG_WOLFSSL_MESSAGES_ON)
        Serial.println(F("Enabling verbose messages wolfSSL_Debugging_ON"));
        wolfSSL_Debugging_ON();
    #else
        Serial.println(F("Enable verbose messages with wolfSSL_Debugging_ON"));
        Serial.println(F("or define DEBUG_WOLFSSL_MESSAGES_ON"));
    #endif
#else
    Serial.println(F("wolfSSL Debugging is Off! (enable with DEBUG_WOLFSSL)"));
#endif

    /* See ssl.c for TLS cache settings. Larger cache = use more RAM. */
#if defined(NO_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS NO_SESSION_CACHE"));
#elif defined(MICRO_SESSION_CACHEx)
    Serial.println(F("wolfSSL TLS MICRO_SESSION_CACHE"));
#elif defined(SMALL_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS SMALL_SESSION_CACHE"));
#elif defined(MEDIUM_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS MEDIUM_SESSION_CACHE"));
#elif defined(BIG_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS BIG_SESSION_CACHE"));
#elif defined(HUGE_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS HUGE_SESSION_CACHE"));
#elif defined(HUGE_SESSION_CACHE)
    Serial.println(F("wolfSSL TLS HUGE_SESSION_CACHE"));
#else
    Serial.println(F("WARNING: Unknown or no TLS session cache setting."));
    /* See wolfssl/src/ssl.c for amount of memory used.
     * It is best on embedded devices to choose a TLS session cache size. */
#endif

     /* Initialize wolfSSL before assigning ctx */
    ret = wolfSSL_Init();
    if (ret == WOLFSSL_SUCCESS) {
        Serial.println("Successfully called wolfSSL_Init");
    }
    else {
        Serial.println("ERROR: wolfSSL_Init failed");
    }

    /* See companion server example with wolfSSLv23_server_method here.
     * method = wolfSSLv23_client_method());   SSL 3.0 - TLS 1.3.
     * method = wolfTLSv1_2_client_method();   only TLS 1.2
     * method = wolfTLSv1_3_client_method();   only TLS 1.3
     *
     * see Arduino\libraries\wolfssl\src\user_settings.h */

    Serial.println("Here we go!");

#ifdef WOLFSSL_DTLS13
        Serial.println(F("Setting wolfDTLSv1_3_client_method"));
        method = wolfDTLSv1_3_client_method();
#else
        Serial.println(F("Setting wolfDTLSv1_2_client_method"));
        method = wolfDTLSv1_2_client_method();
#endif
    ctx = wolfSSL_CTX_new(method);
    if (ctx == NULL) {
        fail_wait();
    }

    if (method == NULL) {
        Serial.println(F("Unable to get wolfssl client method"));
        fail_wait();
    }

    ctx = wolfSSL_CTX_new(method);
    if (ctx == NULL) {
        Serial.println(F("unable to get ctx"));
        fail_wait();
    }

    return ret;
}

/*****************************************************************************/
/* Arduino setup_certificates()                                              */
/*****************************************************************************/
int setup_certificates(void)
{
    int ret = 0;

    /* See user_settings.h that should have included wolfssl/certs_test.h */

    Serial.println(F("Initializing certificates..."));
    show_memory();

    /* Use built-in validation, No verification callback function: */
    wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, 0);

    /* Certificate */
    Serial.println("Initializing certificates...");
    ret = wolfSSL_CTX_use_certificate_buffer(ctx,
                                             CTX_CLIENT_CERT,
                                             CTX_CLIENT_CERT_SIZE,
                                             CTX_CLIENT_CERT_TYPE);
    if (ret == WOLFSSL_SUCCESS) {
        Serial.print("Success: use certificate: ");
        Serial.println(xstr(CTX_SERVER_CERT));
    }
    else {
        Serial.println(F("Error: wolfSSL_CTX_use_certificate_buffer failed: "));
        wc_ErrorString(ret, wc_error_message);
        Serial.println(wc_error_message);
        fail_wait();
    }

    /* Setup private client key */
    ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
                                            CTX_CLIENT_KEY,
                                            CTX_CLIENT_KEY_SIZE,
                                            CTX_CLIENT_KEY_TYPE);
    if (ret == WOLFSSL_SUCCESS) {
        Serial.print("Success: use private key buffer: ");
        Serial.println(xstr(CTX_SERVER_KEY));
    }
    else {
        Serial.println(F("Error: wolfSSL_CTX_use_PrivateKey_buffer failed: "));
        wc_ErrorString(ret, wc_error_message);
        Serial.println(wc_error_message);
        fail_wait();
    }

    ret = wolfSSL_CTX_load_verify_buffer(ctx,
                                         CTX_CA_CERT,
                                         CTX_CA_CERT_SIZE,
                                         CTX_CA_CERT_TYPE);
    if (ret == WOLFSSL_SUCCESS) {
        Serial.println(F("Success: load_verify CTX_CA_CERT"));
    }
    else {
        Serial.println(F("Error: wolfSSL_CTX_load_verify_buffer failed: "));
        wc_ErrorString(ret, wc_error_message);
        Serial.println(wc_error_message);
        fail_wait();
    }

    return ret;
} /* Arduino setup */

/*****************************************************************************/
/*****************************************************************************/
/* Arduino setup()                                                           */
/*****************************************************************************/
/*****************************************************************************/
void setup(void) {
    int i = 0;
    Serial.begin(SERIAL_BAUD);
    while (!Serial && (i < 10)) {
        /* wait for serial port to connect. Needed for native USB port only */
        delay(1000);
        i++;
    }
    Serial.println(F(""));
    Serial.println(F(""));
    Serial.println(F("wolfSSL DTLS Client Example Startup."));

    /* Optionally pre-allocate a large block of memory for testing */
#if defined(MEMORY_STRESS_TEST)
    Serial.println(F("WARNING: Memory Stress Test Active!"));
    Serial.print(F("Allocating extra memory: "));
    Serial.print(MEMORY_STRESS_INITIAL);
    Serial.println(F(" bytes..."));
    memory_stress[mem_ctr] = (char*)malloc(MEMORY_STRESS_INITIAL);
    show_memory();
#endif

    setup_hardware();

    setup_network();

    setup_datetime();

    setup_wolfssl();

    setup_certificates();

#if defined THIS_USER_SETTINGS_VERSION
    Serial.print(F("This user_settings.h version:"))
        Serial.println(THIS_USER_SETTINGS_VERSION)
#endif

    Serial.println(F("Completed Arduino setup!"));
    /* See companion wolfssl_server_dtls.ino code; server begins listening here
     * https://github.com/wolfSSL/wolfssl-examples/tree/master/Arduino/sketches/wolfssl_server_dtls
     * Any other DTLS server will work. See also:
     * https://github.com/wolfSSL/wolfssl/tree/master/examples/client
     */
    return;
} /* Arduino setup */

/*****************************************************************************/
/* wolfSSL error_check()                                                     */
/*****************************************************************************/
int error_check(int this_ret, bool halt_on_error,
                      const __FlashStringHelper* message) {
    int ret = 0;
    if (this_ret == WOLFSSL_SUCCESS) {
        Serial.print(F("Success: "));
        Serial.println(message);
    }
    else {
        Serial.print(F("ERROR: return = "));
        Serial.print(this_ret);
        Serial.print(F(": "));
        Serial.println(message);
        Serial.println(wc_GetErrorString(this_ret));
        if (halt_on_error) {
            fail_wait();
        }
    }
    show_memory();

    return ret;
} /* error_check */

/*****************************************************************************/
/* wolfSSL error_check_ssl                                                   */
/*   Parameters:                                                             */
/*     ssl           is the current WOLFSSL object pointer                   */
/*     halt_on_error set to true to suspend operations for critical error    */
/*     message       is expected to be a memory-efficient F("") macro string */
/*****************************************************************************/
int error_check_ssl(WOLFSSL* ssl, int this_ret, bool halt_on_error,
                           const __FlashStringHelper* message) {
    int err = 0;

    if (ssl == NULL) {
        Serial.println(F("ssl is Null; Unable to allocate SSL object?"));
#ifndef DEBUG_WOLFSSL
        Serial.println(F("Define DEBUG_WOLFSSL in user_settings.h for more."));
#else
        Serial.println(F("See wolfssl/wolfcrypt/error-crypt.h for codes."));
#endif
        Serial.print(F("ERROR: "));
        Serial.println(message);
        show_memory();
        if (halt_on_error) {
            fail_wait();
        }
    }
    else {
        err = wolfSSL_get_error(ssl, this_ret);
        if (err == WOLFSSL_SUCCESS) {
            Serial.print(F("Success m: "));
            Serial.println(message);
        }
        else {
            if (err < 0) {
                wolfSSL_ERR_error_string(err, errBuf);
                Serial.print(F("WOLFSSL Error: "));
                Serial.print(err);
                Serial.print(F("; "));
                Serial.println(errBuf);
            }
            else {
                Serial.println(F("Success: ssl object."));
            }
        }
    }

    return err;
} /* error_check_ssl */

/*****************************************************************************/
/*****************************************************************************/
/* Arduino loop()                                                            */
/*****************************************************************************/
/*****************************************************************************/
void loop()
{
    /* standard variables used in a dtls client */
    char            sendLine[MAXLINE] = "Hello DTLS wolfSSL!";
    char            recvLine[MAXLINE - 1];
    struct          sockaddr_in servAddr;
    const char*     cipherName;
    int             msg_ct = 0;
    int             n = 0;
    int             sockfd = INVALID_SOCKET;
    int             err;
    int             ret;
    int             exitVal = 1;

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        Serial.println(F("unable to get ssl object\n"));
        goto cleanup;
    }

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(WOLFSSL_PORT);
    if (inet_pton(AF_INET, WOLFSSL_DTLS_SERVER_HOST, &servAddr.sin_addr) < 1) {
        perror("inet_pton()");
        goto cleanup;
    }

    if (wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr))
            != WOLFSSL_SUCCESS) {
        Serial.println(F("wolfSSL_dtls_set_peer failed\n"));
        goto cleanup;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
       perror("socket()");
       goto cleanup;
    }

    /* Set the file descriptor for ssl */
    if (wolfSSL_set_fd(ssl, sockfd) != WOLFSSL_SUCCESS) {
        Serial.println(F("cannot set socket file descriptor\n"));
        goto cleanup;
    }

    Serial.print(F("Connecting to wolfSSL DTLS Secure Server..."));
    do {
        reconnect--;
        err = 0; /* reset error */
        Serial.println(F("wolfSSL_connect ..."));
        ret = wolfSSL_connect(ssl);
        if ((ret != WOLFSSL_SUCCESS) && (ret != WC_PENDING_E)) {
            Serial.println(F("Failed connection, checking error."));
            err = error_check_ssl(ssl, ret, true,
                F("Create WOLFSSL object from ctx"));
            Serial.print("err =");
            Serial.println(err);
        }
        else {
            Serial.print(PROGRESS_DOT);
        }
    } while ((err == WC_PENDING_E) && (reconnect > 0));

    Serial.println();
    Serial.println(F("Connected!"));
    Serial.print(F("SSL version is "));
    Serial.println(wolfSSL_get_version(ssl));

    cipherName = wolfSSL_get_cipher(ssl);
    Serial.print(F("SSL cipher suite is "));
    Serial.println(cipherName);

/*****************************************************************************/
/*                  Code for sending datagram to server                      */
/*****************************************************************************/
    Serial.println(F("Begin DTLS Loop..."));
    msg_ct = 0;
    while (msg_ct < DTLS_MESSAGE_CT || (DTLS_MESSAGE_CT == -1)) {
        msg_ct++;

        /* Send sendLine to the server */
        Serial.print(F("Sending Message #"));
        Serial.print(msg_ct);
        Serial.print(F(": \""));
        Serial.print(F(sendLine));
        Serial.println(F("\" ... "));
        if (wolfSSL_write(ssl, sendLine, strlen(sendLine)) != strlen(sendLine)) {
            err = error_check_ssl(ssl, ret, true,
                F("Create WOLFSSL object from ctx"));
            Serial.print("err =");
            Serial.println(err);
            goto cleanup;
        }

        /* n is the # of bytes received */
        Serial.println(F("Reading Message..."));
        n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);

        if (n > 0) {
            /* Add a terminating character to the generic server message */
            recvLine[n] = '\0';
            Serial.println(F("Got Message..."));
            printf("%s\n", recvLine);
        }
        else {
            err = error_check_ssl(ssl, ret, true,
                F("Create WOLFSSL object from ctx"));
            Serial.print("err =");
            Serial.println(err);
            goto cleanup;
        }

    } /*  (msg_ct > DTLS_MESSAGE_CT || (DTLS_MESSAGE_CT == -1)) */

    exitVal = 0;
cleanup:
    if (ssl != NULL) {
        /* Attempt a full shutdown */
        ret = wolfSSL_shutdown(ssl);
        if (ret == WOLFSSL_SHUTDOWN_NOT_DONE) {
            Serial.println("Not done... Try again wolfSSL_shutdown");
            ret = wolfSSL_shutdown(ssl);
        }

        if (ret != WOLFSSL_SUCCESS) {
            err = error_check_ssl(ssl, ret, true,
                F("Create WOLFSSL object from ctx"));
            Serial.print("err =");
            Serial.println(err);
            Serial.println(F("wolfSSL_shutdown failed\n"));
        }
        wolfSSL_free(ssl);
    }
    if (sockfd != INVALID_SOCKET) {
        close(sockfd);
    }
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
    }
    wolfSSL_Cleanup();

    Serial.print(F("Reset to start over."));
    Serial.print(F("Done!"));

    while (1) {
        delay(1000);
    }
} /* Arduino loop */

