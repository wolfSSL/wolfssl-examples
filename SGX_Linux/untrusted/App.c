/* App.c
*
* Copyright (C) 2006-2016 wolfSSL Inc.
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


#include "stdafx.h"
#include "App.h" /* contains include of Enclave_u.h which has wolfSSL header files */
#include "client-tls.h"

/* Use Debug SGX ? */
#if _DEBUG
	#define DEBUG_VALUE SGX_DEBUG_FLAG
#else
	#define DEBUG_VALUE 1
#endif

typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;

int main(int argc, char* argv[]) /* not using since just testing w/ wc_test */
{
	sgx_enclave_id_t id;
	sgx_launch_token_t t;

	int ret = 0;
	int sgxStatus = 0;
	int updated = 0;
    func_args args = { 0 };

	/* only print off if no command line arguments were passed in */
	if (argc == 1) {
		printf("Setting up Enclave ... ");
	}

	memset(t, 0, sizeof(sgx_launch_token_t));

	ret = sgx_create_enclave(ENCLAVE_FILENAME, DEBUG_VALUE, &t, &updated, &id, NULL);
	if (ret != SGX_SUCCESS) {
		printf("Failed to create Enclave : error %d - %#x.\n", ret, ret);
		return 1;
	}

    printf("\nCrypt Test:\n");
    wc_test(id, &sgxStatus, &args);
    printf("Crypt Test: Return code %d\n", args.return_code);
    printf("\n\n\n");

    memset(&args,0,sizeof(args));

    printf("\nBenchmark Test:\n");
    wc_benchmark_test(id, &sgxStatus, &args);
    printf("Benchmark Test: Return code %d\n", args.return_code);
    printf("\n\n\n");

    printf("\nClient Test:\n");
    client_connect(id);

	return 0;
}

static double current_time()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);

	return (double)(1000000 * tv.tv_sec + tv.tv_usec)/1000000.0;
}

void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */ printf("%s", str);
}

void ocall_current_time(double* time)
{
    if(!time) return;
    *time = current_time();
    return;
}

void ocall_low_res_time(int* time)
{
    struct timeval tv;
    if(!time) return;
    *time = tv.tv_sec;
    return;
}

size_t ocall_recv(int sockfd, void *buf, size_t len, int flags)
{
    return recv(sockfd, buf, len, flags);
}

size_t ocall_send(int sockfd, const void *buf, size_t len, int flags)
{
    return send(sockfd, buf, len, flags);
}

