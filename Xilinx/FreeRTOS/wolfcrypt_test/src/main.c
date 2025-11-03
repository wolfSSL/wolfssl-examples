/* main.c
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include "FreeRTOS.h"
#include "task.h"
#include "xil_printf.h"
#include "xrtcpsu.h"

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfcrypt/test/test.h>

#include "main.h"


typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;

static void wolfcrypt_task(void* in) {
	func_args args;
	(void)in;
	wolfcrypt_test(&args);
	xil_printf("done\n\r\n\r");
	vTaskDelete(NULL); /* done so delete self */
}

int main() {
	TaskHandle_t wolfcryptTask;

	XRtcPsu_Config* con;
	XRtcPsu         rtc;

#ifdef WOLFSSL_XILINX_CRYPT
	/* Change from SYSOSC to PLL using the CSU_CTRL register */
	u32 value = Xil_In32(XSECURE_CSU_CTRL_REG);
	Xil_Out32(XSECURE_CSU_CTRL_REG, value | 0x1);

	/* Change the CSU PLL divisor using the CSU_PLL_CTRL register */
	Xil_Out32(0xFF5E00A0, 0x1000400);

	xil_printf("Demonstrating Xilinx hardened crypto\n\r");
#elif defined(WOLFSSL_ARMASM)
	xil_printf("Demonstrating ARMv8 hardware acceleration\n\r");
#else
	xil_printf("Demonstrating wolfSSL software implementation\n\r");
#endif


	con = XRtcPsu_LookupConfig(XPAR_XRTCPSU_0_DEVICE_ID);
	XRtcPsu_CfgInitialize(&rtc, con, con->BaseAddr);
	XRtcPsu_SetTime(&rtc, 1561764163); /* set date to 06/28/2019 */

	/* Demonstration of using wolfSSL software and wolfSSL with xilsecure */

	xTaskCreate(wolfcrypt_task,
			"wolfCrypt tests",
			80000,
			NULL,
			3,
			&wolfcryptTask);

	vTaskStartScheduler();

	for (;;);

	return 0;
}
