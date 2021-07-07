#include <deos.h>
#include <videobuf.h>
#include <timeout.h>
#include <socketapi.h>
#include <lwip-socket.h>
#include <mem.h>
#include <string.h>


void goToSleep()
//**************************************************************************************************
// Function used to loop indefinitely when a failure occurs.  How you handle failure cases depends
// on your system requirements and design.  If you choose to delete the process with the failure,
// the video display destructor would run and any error messages displayed on the video display
// would be lost...therefore, it's better to loop forever.
//**************************************************************************************************
{
  while(1)
	  waitUntilNextPeriod();
}


int setupTransport(clientConnectionHandleType &connectionHandle, char* connectionId)
//**************************************************************************************************
// Function used to set up the mailbox transport for a network connection.
// The steps for setting up the transport layer and establishing a connection with the Network
// Server are identical for UDP and TCP.  This example uses one MTL configuration file to configure
// both connections (UDP and TCP). Within Deos, you are only allowed one MTL config file per process.
// For details on the format and content of the config file refer to Chapter 3. Configuration File
// in the User Guide for the Deos Mailbox Transport Library.
//**************************************************************************************************
{
  int setupStatus, setupError;
  void * sendBuffer;
  uint32_t bufferSizeInBytes;
  VideoStream	VideoOutErr (14, 0, 4, 80);

  if ((setupStatus = socketTransportInitialize("mailbox-transport.config","transportConfigurationId",(uint32_t)waitIndefinitely,&setupError)) != transportSuccess)
    VideoOutErr << "socketTransportInitialize returned 0x" << setupStatus << ", error " << setupError << "\n";
  else if ((setupStatus = socketTransportClientInitialize((uint32_t)waitIndefinitely, &setupError)) != transportSuccess)
    VideoOutErr << "socketTransportClientInitialize returned 0x" << setupStatus << ", error " << setupError << "\n";
  else if ((setupStatus = socketTransportCreateConnection(connectionId, (uint32_t)waitIndefinitely, COMPATIBILITY_ID_2, &connectionHandle, &sendBuffer, &bufferSizeInBytes, &setupError)) != transportSuccess)
    VideoOutErr << "socketTransportCreateConnection returned 0x" << setupStatus << ", error " << setupError << "\n";
  else if ((setupStatus = socketTransportSetConnectionForThread(currentThreadHandle(), connectionHandle, (uint32_t)waitIndefinitely, &setupError)) != transportSuccess)
    VideoOutErr << "socketTransportSetConnectionForThread returned 0x" << setupStatus << ", error " << setupError << "\n";

  return setupStatus;
}

#include <printx.h>

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
//#include <wolfssl/wolfcrypt/logging.h> /* to use WOLFSSL_MSG */

#include "ca_cert.h"
#define MAXLINE 128
#define SERV_PORT 11111

#define close closesocket

void dtls_client(uintData_t)
{
    clientConnectionHandleType UDPconnectionHandle;

    VideoStream VideoOutUDPtitle(3, 0, 1, 80);
    VideoStream VideoOutUDP(4, 0, 10, 80);

    /* standard variables used in a dtls client*/
    int             n = 0;
    int             sockfd = 0;
    int             err1;
    int             readErr;
    struct          sockaddr_in servAddr;
    WOLFSSL*        ssl = 0;
    WOLFSSL_CTX*    ctx = 0;
    char            sendLine[MAXLINE] = "test 1";
    char            recvLine[MAXLINE - 1];
    int             ret = 0;
    uint8_t addr[4] = {192, 168, 86, 55};
    int count = 0;

    // set up the mailbox transport
    if (setupTransport(UDPconnectionHandle, (char*)"connectionId1") != transportSuccess)
    {
        VideoOutUDP << "UDP transport set up failed \n";
        goto done;
    }


    /* Initialize wolfSSL before assigning ctx */
    wolfSSL_Init();

    initPrintx("");
    initPrintxP("");
  
    wolfSSL_Debugging_ON();
    waitUntilNextPeriod();

    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
      VideoOutUDP << "wolfSSL_CTX_new error.\n";
      goto done;
    }
    VideoOutUDP << "wolfSSL_CTX_new \n";

      
    /* Load certificates into ctx variable */
#if 0
    ret = wolfSSL_CTX_load_verify_buffer(ctx,
                                         ca_certs,
                                         sizeof(ca_certs),
                                         SSL_FILETYPE_PEM);
#else
    ret = wolfSSL_CTX_load_verify_buffer_ex(ctx,
                                            ca_certs,
                                            sizeof(ca_certs),
                                            SSL_FILETYPE_PEM,
                                            0,
                                            WOLFSSL_LOAD_FLAG_DATE_ERR_OKAY);
#endif
    
    if(ret != SSL_SUCCESS) {
      VideoOutUDP << "wolfSSL load certs failed\n";
      goto done;
    }
    
    VideoOutUDP << "wolfSSL_CTX_load_verify \n";

    /* Assign ssl variable */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
    	VideoOutUDP << "unable to get ssl object";
        goto done;
    }
    VideoOutUDP << "wolfSSL_new \n";

    /* servAddr setup */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    memcpy(&servAddr.sin_addr, addr, 4);

    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("cannot create a socket.");
        goto done;
    }

    /* Set the file descriptor for ssl and connect with ssl variable */
    wolfSSL_set_fd(ssl, sockfd);
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
        err1 = wolfSSL_get_error(ssl, 0);
        printf("err = %d, %s\n", err1, wolfSSL_ERR_reason_error_string(err1));
        printf("SSL_connect failed");
        goto done;
    }
    VideoOutUDP << "wolfSSL_connect \n";

    /*****************************************************************************/
    /*                  Code for sending datagram to server                      */
    /* Loop until the user is finished */
    while (1) {
        if (count != 0)
            for(int i=0; i < 100; i++)
                waitUntilNextPeriod();

        snprintf(sendLine, MAXLINE, "test %04d", count++);
        /* Send sendLine to the server */
        if ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine)))
             != strlen(sendLine)) {
            printf("SSL_write failed");
        }

        /* n is the # of bytes received */
        n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);

        if (n < 0) {
            readErr = wolfSSL_get_error(ssl, 0);
            if (readErr != SSL_ERROR_WANT_READ) {
                printf("wolfSSL_read failed");
            }
        }

        /* Add a terminating character to the generic server message */
        recvLine[n] = '\0';
        printf("rx: '%s'\n", recvLine);
        VideoOutUDP << "recieved: " << recvLine << "\n";
    }

 done:
    /* Housekeeping */
    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    while(1)
  	  waitUntilNextPeriod();
    return;
}


int main(void)
//***************************************************************************************************
// The main() thread is responsible for creating the UDP and TCP threads...that's it.  So, once these
// two threads are created, we'll delete the main thread so that it's thread budget is returned to
// slack for other processes to use.
//***************************************************************************************************
{
  VideoStream VideoOutMain(0, 0, 3, 80);
  VideoOutMain << "wolfSSL DTLS client Example";

  // taken from hello-world-timer.cpp
  struct tm starttime = { 0, 30, 12, 1, 12, 2020-1900, 0, 0, 0 };
  // startdate: Dev 1 2020, 12:30:00
  struct timespec ts_date;
  ts_date.tv_sec  = mktime(&starttime);
  ts_date.tv_nsec = 0LL;
  int res1 = clock_settime(CLOCK_REALTIME, &ts_date);
  // this will only take effect, if time-control is set in the xml-file
  // if not, Jan 1 1970, 00:00:00 will be the date

  // Create client thread
  thread_handle_t dtls_handle;
  threadStatus ts;
  ts = createThread("DTLSclient", "DTLSThreadTemplate", dtls_client, 0, &dtls_handle );
  if (ts != threadSuccess)
  {
    VideoOutMain << "Unable to create DTLS thread " << (uint32_t)ts << endl;
  }

  // Let's go ahead and delete this thread
  deleteThread(currentThreadHandle());
}
