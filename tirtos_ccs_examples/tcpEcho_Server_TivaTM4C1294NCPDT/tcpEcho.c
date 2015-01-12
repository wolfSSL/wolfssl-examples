/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *    ======== tcpEcho.c ========
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

/* NDK Header files */
#include <sys/socket.h>
#include <ti/ndk/nettools/mytime/mytime.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* Example/Board Header files */
#include "Board.h"

/* wolfSSL Header files */
#include <wolfssl/ssl.h>
#include <wolfssl/certs_test.h>

#define TCPPACKETSIZE 1024
#define TCPPORT 11111
#define NUMTCPWORKERS 3

void exitApp(WOLFSSL_CTX* ctx);

/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function.
 */
Void tcpWorker(UArg arg0, UArg arg1)
{
    int clientfd = 0;
    int nbytes;
    bool flag = true;
    char *buffer;
    Error_Block eb;
    WOLFSSL* ssl = (WOLFSSL *)arg0;

    fdOpenSession(TaskSelf());
    clientfd = wolfSSL_get_fd(ssl);
    System_printf("tcpWorker: start clientfd = 0x%x\n", clientfd);

    /* Make sure Error_Block is initialized */
    Error_init(&eb);

    /* Get a buffer to receive incoming packets. Use the default heap. */
    buffer = Memory_alloc(NULL, TCPPACKETSIZE, 0, &eb);
    if (buffer == NULL) {
        System_printf("tcpWorker: failed to alloc memory\n");
        Task_exit();
    }


    /* Loop while we receive data */
    while (flag) {
    	char ack[] = "wolfSSL's Tiva C Series Connected Launchpad Heard you loud and clear!!! -Kaleb\n";
        nbytes = wolfSSL_read(ssl, (char *)buffer, TCPPACKETSIZE);
        if (nbytes > 0) {
            /* Echo the data back */
            wolfSSL_write(ssl, (char *)ack, strlen(ack));
        }
        else {
            wolfSSL_free(ssl);
            fdClose((SOCKET)clientfd);
            flag = false;
        }
    }
    System_printf("tcpWorker stop clientfd = 0x%x\n", clientfd);

    /* Free the buffer back to the heap */
    Memory_free(NULL, buffer, TCPPACKETSIZE);

    fdCloseSession(TaskSelf());
    /*
     *  Since deleteTerminatedTasks is set in the cfg file,
     *  the Task will be deleted when the idle task runs.
     */
    Task_exit();
}

/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
Void tcpHandler(UArg arg0, UArg arg1)
{
    int lSocket;
    struct sockaddr_in sLocalAddr;
    int clientfd;
    struct sockaddr_in client_addr;
    int addrlen=sizeof(client_addr);
    int optval;
    int optlen = sizeof(optval);
    int status;
    Task_Handle taskHandle;
    Task_Params taskParams;
    Error_Block eb;

    fdOpenSession(TaskSelf());

    wolfSSL_Init();
    WOLFSSL_CTX* ctx = NULL;

    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method())) == NULL) {
       System_printf("tcpHandler: wolfSSL_CTX_new error.\n");
       exitApp(ctx);
    }

    if (wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
                                  sizeof(ca_cert_der_2048)/sizeof(char),
                                  SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
       System_printf("tcpHandler: Error loading ca_cert_der_2048"
                     " please check the wolfssl/certs_test.h file.\n");
       exitApp(ctx);
    }

    if (wolfSSL_CTX_use_certificate_buffer(ctx, server_cert_der_2048,
                                  sizeof(server_cert_der_2048)/sizeof(char),
                                  SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
       System_printf("tcpHandler: Error loading server_cert_der_2048,"
                     " please check the wolfssl/certs_test.h file.\n");
       exitApp(ctx);
    }

    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, server_key_der_2048,
                                  sizeof(server_key_der_2048)/sizeof(char),
                                  SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
       System_printf("tcpHandler: Error loading server_key_der_2048,"
                     " please check the wolfssl/certs_test.h file.\n");
       exitApp(ctx);
    }

    lSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lSocket == -1) {
        System_printf("tcpHandler: socket failed\n");
        Task_exit();
    }

    memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
    sLocalAddr.sin_family = AF_INET;
    sLocalAddr.sin_len = sizeof(sLocalAddr);
    sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sLocalAddr.sin_port = htons(arg0);

    status = bind(lSocket, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr));
    if (status < 0) {
        System_printf("tcpHandler: bind failed\n");
        fdClose((SOCKET)lSocket);
        exitApp(ctx);
    }

    if (listen(lSocket, NUMTCPWORKERS) != 0){
        System_printf("tcpHandler: listen failed\n");
        fdClose((SOCKET)lSocket);
        exitApp(ctx);
    }

    if (setsockopt(lSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        System_printf("tcpHandler: setsockopt failed\n");
        fdClose((SOCKET)lSocket);
        exitApp(ctx);
    }

    while (true) {
         WOLFSSL* ssl;

        /* Wait for incoming request */
        if ((clientfd = accept(lSocket, (struct sockaddr*)&client_addr,
            &addrlen)) == -1) {
            System_printf("tcpHandler: Accept failed %d\n");
            exitApp(ctx);
        }

        /* Init the Error_Block */
        Error_init(&eb);
        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            System_printf("tcpHandler: wolfSSL_new error.\n");
            exitApp(ctx);
        }

        wolfSSL_set_fd(ssl, clientfd);

        /* Initialize the defaults and set the parameters. */
        Task_Params_init(&taskParams);
        taskParams.arg0 = (UArg)ssl;
        taskParams.stackSize = 16384;
        taskHandle = Task_create((Task_FuncPtr)tcpWorker, &taskParams, &eb);
        if (taskHandle == NULL) {
            System_printf("tcpHandler: Failed to create new Task\n");
        }
    }
}

/*
 *  ======== exitApp ========
 *  Cleans up the SSL context and exits the application
 */
void exitApp(WOLFSSL_CTX* ctx)
{
    if (ctx != NULL) {
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
    }

     BIOS_exit(-1);
}

/*
 *  ======== main ========
 */
int main(void)
{
    Task_Handle taskHandle;
    Task_Params taskParams;
    Error_Block eb;
#ifdef TIVAWARE
    /*
     *  This is a work-around for EMAC initialization issues found on
     *  the TM4C129 devices. The bug number is:
     *  SDOCM00107378: NDK examples for EK-TM4C1294XL do not work
     *
     *  The following disables the flash pre-fetch. It is enable within the
     *  EMAC driver (in the EMACSnow_NIMUInit() function).
     */
    UInt32 ui32FlashConf;

    ui32FlashConf = HWREG(0x400FDFC8);
    ui32FlashConf &= ~(0x00020000);
    ui32FlashConf |= 0x00010000;
    HWREG(0x400FDFC8) = ui32FlashConf;
#endif

    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initEMAC();

    /*
     * wolfSSL library needs time() for validating certificates.
     * USER STEP: Set up the current time in seconds below.
     */
    MYTIME_init();
    MYTIME_settime(1398980099);

    System_printf("Starting the TCP Echo example\nSystem provider is set to "
                  "SysMin. Halt the target to view any SysMin contents in"
                  " ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */
    System_flush();

    /*
     *  Create the Task that farms out incoming TCP connections.
     *  arg0 will be the port that this task listens to.
     */
    Task_Params_init(&taskParams);
    Error_init(&eb);

    taskParams.stackSize = 8192;
    taskParams.priority = 1;
    taskParams.arg0 = TCPPORT;
    taskHandle = Task_create((Task_FuncPtr)tcpHandler, &taskParams, &eb);
    if (taskHandle == NULL) {
        System_printf("main: Failed to create tcpHandler Task\n");
    }

    /* Start BIOS */
    BIOS_start();

    return (0);
}
