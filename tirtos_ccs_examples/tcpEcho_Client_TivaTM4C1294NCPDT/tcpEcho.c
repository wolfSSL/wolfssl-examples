/*
 * This file contains contributions from both TI and wolfSSL
 *
 * Copyright (c) 2014, Texas Instruments Incorporated
 *
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
 *
 * Copyright (c) 2006-2020, wolfSSL Inc.
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

/*
 *    ======== tcpClient.c ========
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
#include <ti/ndk/inc/nettools/inc/inet.h>

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

/* Set to the IP of the computer running the wolfssl command from the 
 * wolfssl_root directory: "./examples/server/server -b" 
 * the "-b" tells the server to bind to any interface, not just 127.0.0.1
 */
#define IP_ADDR "xxx.xxx.x.xxx"

void exitApp(WOLFSSL_CTX* ctx);

/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
Void tcpHandler(UArg arg0, UArg arg1) {
	int sockfd;
	int ret;
	struct sockaddr_in servAddr;
	Error_Block eb;
	bool flag = true;
	bool internal_flag = true;
	int nbytes;
	char *buffer;
	char msg[] = "Hello from TM4C1294XL Connected Launchpad";
	WOLFSSL* ssl = (WOLFSSL *) arg0;

	fdOpenSession(TaskSelf());

	wolfSSL_Init();
	WOLFSSL_CTX* ctx = NULL;

	ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
	if (ctx == 0) {
		System_printf("tcpHandler: wolfSSL_CTX_new error.\n");
		exitApp(ctx);
	}

	if (wolfSSL_CTX_load_verify_buffer(ctx, ca_cert_der_2048,
			sizeof(ca_cert_der_2048) / sizeof(char), SSL_FILETYPE_ASN1)
			!= SSL_SUCCESS) {
		System_printf("tcpHandler: Error loading ca_cert_der_2048"
				" please check the wolfssl/certs_test.h file.\n");
		exitApp(ctx);
	}

	if (wolfSSL_CTX_use_certificate_buffer(ctx, client_cert_der_2048,
			sizeof(client_cert_der_2048) / sizeof(char), SSL_FILETYPE_ASN1)
			!= SSL_SUCCESS) {
		System_printf("tcpHandler: Error loading client_cert_der_2048,"
				" please check the wolfssl/certs_test.h file.\n");
		exitApp(ctx);
	}

	if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, client_key_der_2048,
			sizeof(client_key_der_2048) / sizeof(char), SSL_FILETYPE_ASN1)
			!= SSL_SUCCESS) {
		System_printf("tcpHandler: Error loading client_key_der_2048,"
				" please check the wolfssl/certs_test.h file.\n");
		exitApp(ctx);
	}

	/* Init the Error_Block */
	Error_init(&eb);

	do {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			System_printf("tcpHandler: socket failed\n");
			Task_sleep(2000);
			continue;
		}

		memset((char *) &servAddr, 0, sizeof(servAddr));
		servAddr.sin_family = AF_INET;
		servAddr.sin_port = htons(TCPPORT);

		inet_aton(IP_ADDR, &servAddr.sin_addr);

		ret = connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));

		if (ret < 0) {
			fdClose((SOCKET) sockfd);
			Task_sleep(2000);
			continue;
		}
	} while (ret != 0);

	if ((ssl = wolfSSL_new(ctx)) == NULL) {
		System_printf("tcpHandler: wolfSSL_new error.\n");
		exitApp(ctx);
	}

	wolfSSL_set_fd(ssl, sockfd);

	ret = wolfSSL_connect(ssl);

	/* Delete "TOP_LINE" and "END_LINE" for debugging. */

	/* TOP_LINE

	 System_printf("looked for: %d.\n", SSL_SUCCESS);
	 System_printf("return was: %d.\n", ret);
	 int err;
	 char err_buffer[80];
	 err = wolfSSL_get_error(ssl, 0);
	 System_printf("wolfSSL error: %d\n", err);
	 System_printf("wolfSSL error string: %s\n", wolfSSL_ERR_error_string(err, err_buffer));

	 END_LINE */

	if (ret == SSL_SUCCESS) {

		sockfd = wolfSSL_get_fd(ssl);

		/* Get a buffer to receive incoming packets. Use the default heap. */
		buffer = Memory_alloc(NULL, TCPPACKETSIZE, 0, &eb);

		if (buffer == NULL) {
			System_printf("tcpWorker: failed to alloc memory\n");
			exitApp(ctx);
		}

		/* Say hello to the server */
		while (flag) {
			if (wolfSSL_write(ssl, msg, strlen(msg)) != strlen(msg)) {
				ret = wolfSSL_get_error(ssl, 0);
				System_printf("Write error: %i.\n", ret);
			}
			while (internal_flag) {
				nbytes = wolfSSL_read(ssl, (char *) buffer, TCPPACKETSIZE);
				if (nbytes > 0) {
					internal_flag = false;
				}
			}
			/* success */
			System_printf("Heard: \"%s\".\n", buffer);
			wolfSSL_free(ssl);
			fdClose((SOCKET) sockfd);
			flag = false;
		}

		/* Free the buffer back to the heap */
		Memory_free(NULL, buffer, TCPPACKETSIZE);

		/*
		 *  Since deleteTerminatedTasks is set in the cfg file,
		 *  the Task will be deleted when the idle task runs.
		 */
		exitApp(ctx);

	} else {
		wolfSSL_free(ssl);
		fdClose((SOCKET) sockfd);
		System_printf("wolfSSL_connect failed.\n");
		fdCloseSession(TaskSelf());
		exitApp(ctx);
	}
}

/*
 *  ======== exitApp ========
 *  Cleans up the SSL context and exits the application
 */
void exitApp(WOLFSSL_CTX* ctx) {
	if (ctx != NULL) {
		wolfSSL_CTX_free(ctx);
		wolfSSL_Cleanup();
	}

	BIOS_exit(-1);
}

/*
 *  ======== main ========
 */
int main(void) {
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
	MYTIME_settime(1408053541);

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

	taskParams.stackSize = 32768;
	taskParams.priority = 1;
	taskParams.arg0 = TCPPORT;
	taskHandle = Task_create((Task_FuncPtr) tcpHandler, &taskParams, &eb);
	if (taskHandle == NULL) {
		System_printf("main: Failed to create tcpHandler Task\n");
	}

	/* Start BIOS */
	BIOS_start();

	return (0);
}
