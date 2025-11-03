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

#include <wolfssl/wolfcrypt/settings.h>
#include <wolfcrypt/benchmark/benchmark.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/certs_test.h>

#include "FreeRtosConfig.h"
#include "xil_cache.h"
#include "task.h"
#include "main.h"
#include <stdio.h>

extern double current_time(int); /* defined in wolfssl/benchmark/benchmark.c */

typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;

#ifndef NO_RSA

static void bench_rsa4096()
{
    RsaKey      rsaKey;
    WC_RNG      rng;

    unsigned char message[RSA_BUF_SIZE];
    unsigned char enc[RSA_BUF_SIZE];
    const char* messageStr = "Everyone gets Friday off.";
    double      start = 0.0f;
    const int   len = (int)XSTRLEN((char*)messageStr);
    int         rsaKeySz = RSA_BUF_SIZE * 8; /* used in printf */
    int         ntimes = 100;
    int         ret, i;
    word32      idx = 0;

    printf("Benchmarking call to wolfSSL API with RSA 4096 bit key\r\n");
    /* init key */
    if ((ret = wc_InitRsaKey_ex(&rsaKey, NULL, 0)) < 0)
    	goto exit_rsa_pub;

    /* init rng */
    if ((ret = wc_InitRng(&rng)) < 0)
    	goto exit_rsa_pub;

    #ifdef WC_RSA_BLINDING
        ret = wc_RsaSetRNG(&rsaKey, &rng);
        if (ret != 0)
            goto exit_rsa_pub;
    #endif

    /* decode the private key */
    idx = 0;
    if ((ret = wc_RsaPrivateKeyDecode(ssk, &idx, &rsaKey, (word32)sizeof(ssk))) != 0) {
        printf("wc_RsaPrivateKeyDecode failed! %d\n", ret);
        goto exit_rsa_pub;
    }


    /* begin public RSA */
    start = current_time(1);
    for (i = 0; i < ntimes; i++) {
        ret = wc_RsaPublicEncrypt(message, len, enc,
                                            RSA_BUF_SIZE, &rsaKey, &rng);
    } /* for times */
    start = current_time(0) - start;
    printf("RSA %d public : %d ops took %8.3f sec, avg %8.3f ms %8.3f ops/sec \r\n", rsaKeySz, ntimes, start, start * (1/ (double)ntimes) * 1000, ntimes/start);
    exit_rsa_pub:
	if (ret < 0) {
        printf("Public encrypt error! %d\r\n", ret);
    }

	idx = (word32)ret;
    /* begin public RSA */
    start = current_time(1);
    for (i = 0; i < ntimes; i++) {
        ret = wc_RsaPrivateDecrypt(enc, idx, message,
                                            RSA_BUF_SIZE, &rsaKey);
    } /* for times */
    start = current_time(0) - start;
    printf("RSA %d private : %d ops took %8.3f sec, avg %8.3f ms %8.3f ops/sec \r\n", rsaKeySz, ntimes, start, start * (1/ (double)ntimes) * 1000, ntimes/start);

	if (ret < 0) {
        printf("Private decrypt error! %d\r\n", ret);
    }
    wc_FreeRsaKey(&rsaKey);

}

static void bench_rsa2048()
{
    RsaKey      rsaKey;
    WC_RNG      rng;

    unsigned char message[RSA_BUF_SIZE/2];
    unsigned char enc[RSA_BUF_SIZE/2];
    const char* messageStr = "Everyone gets Friday off.";
    double      start = 0.0f;
    const int   len = (int)XSTRLEN((char*)messageStr);
    int         rsaKeySz = RSA_BUF_SIZE/2 * 8; /* used in printf */
    int         ntimes = 100;
    int         ret, i;
    word32      idx = 0;

    printf("Benchmarking call to wolfSSL API with RSA 2048 bit key\r\n");
    /* init key */
    if ((ret = wc_InitRsaKey_ex(&rsaKey, NULL, 0)) < 0)
    	goto exit_rsa_pub;

    /* init rng */
    if ((ret = wc_InitRng(&rng)) < 0)
    	goto exit_rsa_pub;

    #ifdef WC_RSA_BLINDING
        ret = wc_RsaSetRNG(&rsaKey, &rng);
        if (ret != 0)
            goto exit_rsa_pub;
    #endif

    /* decode the private key */
    idx = 0;
    if ((ret = wc_RsaPublicKeyDecode(client_keypub_der_2048, &idx, &rsaKey, sizeof_client_keypub_der_2048)) != 0) {
        printf("wc_RsaPrivateKeyDecode failed! %d\n", ret);
        goto exit_rsa_pub;
    }


    /* begin public RSA */
    start = current_time(1);
    for (i = 0; i < ntimes; i++) {
        ret = wc_RsaPublicEncrypt(message, len, enc,
                                            RSA_BUF_SIZE, &rsaKey, &rng);
    } /* for times */
    start = current_time(0) - start;
    printf("RSA %d public : %d ops took %8.3f sec, avg %8.3f ms %8.3f ops/sec \r\n", rsaKeySz, ntimes, start, start * (1/ (double)ntimes) * 1000, ntimes/start);
    exit_rsa_pub:
	if (ret < 0) {
        printf("Public encrypt error! %d\r\n", ret);
    }

    wc_FreeRsaKey(&rsaKey);

}
#endif /* !NO_RSA */


#ifdef WOLFSSL_XILINX_CRYPT
/* Used for getting sanity benchmark value with calling xsecure API */

#include "xparameters.h"
#include "xsecure_aes.h"
#include "xsecure_rsa.h"

/* this function was used to bench direct xsecure API calls */
static void unchagned()
{
	XCsuDma_Config *Config;
	XSecure_Aes Secure_Aes;
	XSecure_Rsa Secure_Rsa;
	XCsuDma CsuDma;

	int size[] = {16,528,1024,4112,7696, 15888, 65536, 524288};
	int times = 1000;
	int i, j;
	unsigned char buf[524288 + 16];
	//unsigned char tmp[3000000 + 16];
	unsigned int csu_iv[4];
	unsigned int csu_key[32];
    double start = 0, total = 0;
	int Status;

	xil_printf("\r\n********************************************\r\n");
	xil_printf("************ Xilinx API called directly\r\n");
	xil_printf("********************************************\r\n");

	printf("\r\nGet benchmark values of calling xsecure API directly\r\n");
	printf("AES-GCM benchmark of calls to XSecure_AesEncryptData\r\n");

	Config = XCsuDma_LookupConfig(0);
	if (NULL == Config) {
		xil_printf("config  failed \n\r");
		return;
	}

	Status = XCsuDma_CfgInitialize(&CsuDma, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("cfg init failed \n\r");
		return;
	}

	XSecure_AesInitialize(&Secure_Aes, &CsuDma, XSECURE_CSU_AES_KEY_SRC_KUP,
        csu_iv, csu_key);

	for (j = 0; j < 8; j++) {
		times = 0;
		start = current_time(1);
		do {
			XSecure_AesEncryptData(&Secure_Aes, buf, buf, size[j]);
			times++;
		} while ((current_time(0) - start) < 1);
		total = current_time(0) - start;

		printf("%d block size : %d bytes took %8.3f seconds,  %8.3f MB/s\n\r", size[j], times*size[j], total, (times*size[j]/(1024 * 1024))*(1/total));
	}


	/* direct RSA calls */
	XSecure_RsaInitialize(&Secure_Rsa, n, NULL, e);

	times = 100;
	Status = XST_SUCCESS;
	start = 0; total = 0;
	start = current_time(1);
	for (i = 0; i < times; i++) {
	 	 Status |= XSecure_RsaDecrypt(&Secure_Rsa, open_hello_sig, buf);
	}
	total = current_time(0) - start;
	printf("\r\nRSA 4096 bit : %d ops took %8.3f seconds with an average of %8.3f ms and %8.3f ops/sec \r\n", times, total, total * (1/ (double)times) * 1000, (double)times/total);

	if (Status != XST_SUCCESS){
		printf("Error with RSA decrypt\r\n");
	}
	printf("done\r\n");
}
#endif /* WOLFSSL_XILINX_ CRYPT */


static void benchmark_task(void* in) {
	func_args args;
	int i;
	int size[] = {16,528,1024,4112,7696, 15888, 65536, 131072};
	(void)in;


#if configTICK_RATE_HZ != 1000
    #error Timer in benchmark expects tick rate to be set to 1000
#else

	while (1) {
#ifdef WOLFSSL_XILINX_CRYPT
	xil_printf("Demonstrating Xilinx hardened crypto\n\r");
#elif defined(WOLFSSL_ARMASM)
	xil_printf("Demonstrating ARMv8 hardware acceleration\n\r");
#else
	xil_printf("Demonstrating wolfSSL software implementation\n\r");
#endif

#ifdef WOLFSSL_XILINX_CRYPT
	/* Change from SYSOSC to PLL using the CSU_CTRL register */
	u32 value = Xil_In32(XSECURE_CSU_CTRL_REG);
	Xil_Out32(XSECURE_CSU_CTRL_REG, value | 0x1);

	/* Change the CSU PLL divisor using the CSU_PLL_CTRL register */
	Xil_Out32(0xFF5E00A0, 0x1000400);

	unchagned();
#endif

#ifndef NO_RSA
	/* Get benchmark for 4096 bit key because by default benchmark.c benchs 2048 bit key */
	xil_printf("\r\n********************************************\r\n");
	xil_printf("************ Using wc_RsaPublicKeyDecode\n\r");
	xil_printf("********************************************\r\n");

	bench_rsa4096();
	bench_rsa2048(); /* use public decode for benchmark */
#endif
	for (i = 0; i < 6; i++) {
		xil_printf("\r\n********************************************\r\n");
		xil_printf("************ block size of %d\n\r", size[i]);
		xil_printf("********************************************\r\n");
		benchmark_configure(size[i]);
		benchmark_test(&args);
		printf("done\n");
	}
#endif

	}

	vTaskDelete(NULL); /* done so delete self */
}


int main() {
	TaskHandle_t benchTask;
	BaseType_t ret;

	ret = xTaskCreate(benchmark_task,
			"wolfSSL Benchmarking",
			600000,
			NULL,
			configMAX_PRIORITIES - 1,
			&benchTask);

	if (ret != pdPASS) {
		printf("Unable to create benchmark task, check memory size settings.\n");
	}

	vTaskStartScheduler();
	for (;;);

	return 0;
}
