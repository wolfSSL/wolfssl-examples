/* Benchmarks.cpp
*
* Copyright (C) 2006-2020 wolfSSL Inc.
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
#include "Benchmarks.h" /* contains include of Enclave_u.h which has wolfSSL header files */

/* Check settings of wolfSSL */
#if !defined(HAVE_AESGCM) || defined(NO_RSA) || defined(NO_SHA256)
#error please enable AES-GCM, RSA, and SHA256
#endif

/* Use Debug SGX ? */
#if _DEBUG
	#define DEBUG_VALUE SGX_DEBUG_FLAG
#else
	#define DEBUG_VALUE 1
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


/* Choose AES Key size  */
//#define WC_AES_KEY_SZ 16 /* 128 bit key */
#define WC_AES_KEY_SZ 24 /* 192 bit key */

static double current_time(int reset)
{
	static int init = 0;
	static LARGE_INTEGER freq;
	LARGE_INTEGER count;

	(void)reset;

	if (!init) {
		QueryPerformanceFrequency(&freq);
		init = 1;
	}

	QueryPerformanceCounter(&count);
	return (double)count.QuadPart / freq.QuadPart;
}


static void free_resources(byte* plain, byte* cipher) {
	delete[] plain;
	delete[] cipher;
}


/* benchmark is performed calling into Enclave on each update
 * This function tests speeds at different message sizes during update */
static double sha256_getTime_multiple(sgx_enclave_id_t id, double* total) {
	double start, end;
	int ret, sgxStatus;
	byte* plain;
	byte digest[64];
	int plainSz = (1024 * 1024);
	int tSz = (1024 * 1024) * numBlocks;
	int i, k;

	Sha256 sha256;

	ret = 0;
	k = numBlocks;
	printf("\n");
	for (k = 1; k <= numBlocks; k++) {
		plainSz = tSz / k;
		plain = new byte[plainSz];
		ret |= wc_sha256_init(id, &sgxStatus, &sha256);
		start = current_time(1);
		for (i = 0; i < k; i++) {
			ret |= wc_sha256_update(id, &sgxStatus, &sha256, plain, plainSz);
		}
		ret |= wc_sha256_final(id, &sgxStatus, &sha256, digest);
		end = current_time(0);

		if (ret != SGX_SUCCESS || sgxStatus != 0) {
			printf("Error in SHA256 operation with Enclave: %d sgxStatus = %d.\n", ret, sgxStatus);
			return -1;
		}
		*total = end - start;
		printf("%8.3f\n", *total);
		delete[] plain;
	}
	printf("\n");
	*total = end - start;
	return 1 / *total * numBlocks;
}


/* benchmark is performed calling into Enclave on each update */
static double sha256_getTime(sgx_enclave_id_t id, double* total) {
	double start, end;
	int ret = 0;
	int sgxStatus = 0;
	int i;
	byte* plain;
	byte digest[64];
	int plainSz = (1024 * 1024);

	Sha256 sha256;

	plain = new byte[plainSz];
	ret |= wc_sha256_init(id, &sgxStatus, &sha256);
	start = current_time(1);
	
	/* perform work and get digest */
	for (i = 0; i < numBlocks; i++) {
		ret |= wc_sha256_update(id, &sgxStatus, &sha256, plain, plainSz);
	}
	ret |= wc_sha256_final(id, &sgxStatus, &sha256, digest);
	end = current_time(0);

	delete[] plain;
	if (ret != SGX_SUCCESS || sgxStatus != 0) {
		printf("Error in SHA256 operation with Enclave: %d sgxStatus = %d.\n", ret, sgxStatus);
		return -1;
	}

	*total = end - start;
	return 1 / *total * numBlocks;
}


static int sha256_print(sgx_enclave_id_t id)
{
	double total, persec;

	printf("SHA-256           ");
	persec = sha256_getTime(id, &total);
	printf("%d megs took %5.3f seconds , %8.3f MB/s\n", numBlocks, total, persec);

	return 0;
}


/* return time in MB/s with crossing into enclave boundary with each encrypt */
static double aesgcm_encrypt_getTime(sgx_enclave_id_t id, double* total, byte* plain, byte* cipher, word32 sz, byte* tag, word32 tagSz)
{
	Aes aes;
	double start, end;
	int i;
	int ret, sgxStatus;

	const byte ad[13] = { 0 };

	const XGEN_ALIGN byte iv[] =
	{
		0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81
	};

	const XGEN_ALIGN byte key[] =
	{
		0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		0xfe, 0xde, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
		0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67
	};

	ret = wc_aesgcm_setKey(id, &sgxStatus, &aes, key, WC_AES_KEY_SZ);
	if (ret != SGX_SUCCESS || sgxStatus != 0) {
		printf("AES set key failed %d sgxStatus = %d\n", ret, sgxStatus);
		return -1;
	}

	start = current_time(1);
	for (i = 0; i < numBlocks; i++) {
		ret = wc_aesgcm_encrypt(id, &sgxStatus, &aes, cipher, plain, sz, iv, 12, tag, tagSz, ad, 13);
	}
	end = current_time(0);

	if (ret != SGX_SUCCESS || sgxStatus != 0) {
		printf("Error in AES-GCM encrypt operation with Enclave: %d sgxStatus = %d.\n", ret, sgxStatus);
		return -1;
	}

	*total = end - start;
	return 1 / *total * numBlocks;
}


static int aesgcm_encrypt_print(sgx_enclave_id_t id, byte* plain, byte* cipher, word32 sz, byte* tag, word32 tagSz)
{
	double total, persec;

	printf("AES-GCM encrypt   ");
	persec = aesgcm_encrypt_getTime(id, &total, plain, cipher, sz, tag, tagSz);
	printf("%d megs took %5.3f seconds , %8.3f MB/s\n", numBlocks, total, persec);

	return 0;
}


/* return MB/s with crossing into Enclave boundary with each decrypt */
static double aesgcm_decrypt_getTime(sgx_enclave_id_t id, double* total, byte* plain, const byte* cipher, word32 sz, const byte* tag, word32 tagSz)
{
	Aes aes;
	double start, end;
	int ret, sgxStatus;
	int i;

	const byte ad[13] = { 0 };

	const XGEN_ALIGN byte iv[] =
	{
		0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81
	};

	const XGEN_ALIGN byte key[] =
	{
		0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		0xfe, 0xde, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
		0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67
	};

	ret = wc_aesgcm_setKey(id, &sgxStatus, &aes, key, WC_AES_KEY_SZ);
	if (ret != SGX_SUCCESS || sgxStatus != 0) {
		printf("AES set key failed %d sgxStatus = %d\n", ret, sgxStatus);
		return -1;
	}

	start = current_time(1);
	for (i = 0; i < numBlocks; i++) {
		ret = wc_aesgcm_decrypt(id, &sgxStatus, &aes, plain, cipher, sz, iv, 12, tag, tagSz, ad, 13);
	}
	end = current_time(0);

	if (ret != SGX_SUCCESS || sgxStatus < 0) {
		printf("Error in AES-GCM decrypt operation with Enclave: %d sgxStatus = %d.\n", ret, sgxStatus);
		return -1;
	}

	*total = end - start;
	return 1 / *total * numBlocks;
}


static int aesgcm_decrypt_print(sgx_enclave_id_t id, byte* plain, byte* cipher, word32 sz, const byte* tag, word32 tagSz)
{
	double total, persec;

	printf("AES-GCM decrypt   ");
	persec = aesgcm_decrypt_getTime(id, &total, plain, cipher, sz, tag, tagSz);
	printf("%d megs took %5.3f seconds , %8.3f MB/s\n", numBlocks, total, persec);

	return 0;
}


/* return time for each in milliseconds */
static double rsa_encrypt_getTime(sgx_enclave_id_t id, int* sgxStatus, double* total, const byte* message, word32 mSz, byte* cipher, word32 cSz)
{
	double start, end;
	int ret, i;
	int freeStatus = 0;
	RsaKey rsa;

	ret = 0; *sgxStatus = 0;
	ret = wc_rsa_init(id, sgxStatus, &rsa); /* loads RSA key from buffer and inits RNG */
	if (ret != SGX_SUCCESS || *sgxStatus != 0) {
		printf("Initializing RSA failed %d sgxStatus = %d\n", ret, *sgxStatus);
		return -1;
	}

	start = current_time(1);
	for (i = 0; i < ntimes; i++) {
		ret = wc_rsa_encrypt(id, sgxStatus, message, mSz, cipher, cSz, &rsa);
	}
	end = current_time(0);

	if (ret != SGX_SUCCESS || *sgxStatus < 0) {
		printf("Error in rsa encrypt operation with Enclave: %d sgxStatus = %d.\n", ret, *sgxStatus);
		return -1;
	}

	ret = wc_rsa_free(id, &freeStatus, &rsa);
	if (ret != 0 || freeStatus != 0) {
		printf("Failed to free RSA key %d sgxStatus = %d\n", ret, freeStatus);
		return -1;
	}

	*total = end - start;
	return (*total / ntimes) * 1000;
}


static int rsa_encrypt_print(sgx_enclave_id_t id, int* sgxStatus, byte* plain, word32 pSz, byte* cipher, word32 cSz)
{
	double total, each;

	printf("RSA-2048 encrypt   ");
	each = rsa_encrypt_getTime(id, sgxStatus, &total, plain, pSz, cipher, cSz);
	printf("took %6.3f milliseconds, avg over %d\n", each, ntimes);

	return 0;
}


/* return time in milliseconds for each */
static double rsa_decrypt_getTime(sgx_enclave_id_t id, double* total, byte* m, word32 mSz, const byte* c, word32 cSz)
{
	double start, end;
	int ret, sgxStatus, i;
	RsaKey rsa;

	ret = wc_rsa_init(id, &sgxStatus, &rsa); /* loads RSA key from buffer and inits RNG */
	if (ret != SGX_SUCCESS || sgxStatus != 0) {
		printf("Initializing RSA failed %d sgxStatus = %d\n", ret, sgxStatus);
		return -1;
	}

	start = current_time(1);
	for (i = 0; i < ntimes; i++) {
		ret = wc_rsa_decrypt(id, &sgxStatus, c, cSz, m, mSz, &rsa);
	}
	end = current_time(0);

	if (ret != SGX_SUCCESS || sgxStatus < 0) {
		printf("Error in rsa decrypt operation with Enclave: %d sgxStatus = %d.\n", ret, sgxStatus);
		return -1;
	}

	ret = wc_rsa_free(id, &sgxStatus, &rsa);
	if (ret != 0 || sgxStatus != 0) {
		printf("Failed to free RSA key %d sgxStatus = %d\n", ret, sgxStatus);
		return -1;
	}

	*total = end - start;
	return (*total / ntimes) * 1000;
}


static int rsa_decrypt_print(sgx_enclave_id_t id, byte* m, word32 mSz, const byte* c, word32 cSz)
{
	double total, each;

	printf("RSA-2048 decrypt   ");
	each = rsa_decrypt_getTime(id, &total, m, mSz, c, cSz);
	printf("took %6.3f milliseconds, avg over %d\n", each, ntimes);

	return 0;
}

int main(int argc, char* argv[])
{
	sgx_enclave_id_t id;
	sgx_launch_token_t t;

	int ret = 0;
	int sgxStatus = 0;
	int updated = 0;
	byte message[] = "Secure wolfSSL.";

	byte* plain  = new byte[1024 * 1024];
	byte* cipher = new byte[1024 * 1024];
	const byte tag[16] = { 0 };
	int plainSz = 1024 * 1024;
	int i;

	/* only print off if no command line arguments were passed in */
	if (argc == 1) {
		printf("Setting up Enclave ... ");
	}

	memset(t, 0, sizeof(sgx_launch_token_t));

	ret = sgx_create_enclave(_T("Enclave.signed.dll"), DEBUG_VALUE, &t, &updated, &id, NULL);
	if (ret != SGX_SUCCESS) {
		printf("Failed to create Enclave : error %d - %#x.\n", ret, ret);
		free_resources(plain, cipher);
		return 1;
	}

	/* test if only printing off times */
	if (argc > 1) {
		double total;
		int idx = 1;
		while (1) {
			for (idx = 1; idx < argc; idx++) {
				if (strncmp(argv[idx], "-s256", 6) == 0) {
					printf("%8.3f - SHA-256\n", sha256_getTime(id, &total));
					fflush(stdout);
				}
				else if (strncmp(argv[idx], "-ag", 3) == 0) {
					printf("%8.3f - AES-GCM\n", aesgcm_encrypt_getTime(id, &total, plain, cipher, plainSz, (byte*)tag, sizeof(tag)));
					fflush(stdout);
				}
				else if (strncmp(argv[idx], "-re", 4) == 0) {
					printf("%8.3f - RSA-ENC\n", rsa_encrypt_getTime(id, &sgxStatus, &total, message, sizeof(message), cipher, plainSz));
					fflush(stdout);
				}
				else if (strncmp(argv[idx], "-rd", 4) == 0) {
					rsa_encrypt_getTime(id, &sgxStatus, &total, message, sizeof(message), cipher, plainSz);
					printf("%8.3f - RSA-DEC\n", rsa_decrypt_getTime(id, &total, plain, plainSz, cipher, sgxStatus));
					fflush(stdout);
				}
				else {
					printf("\"%s\" Not yet implemented\n\t-s256 for SHA256\n\t-ag for AES-GCM\n\t-re for RSA encrypt", argv[idx]);
					fflush(stdout);
					free_resources(plain, cipher);
					return 0;
				}
			}
		}
	}

	printf("Success\nCollecting benchmark values for wolfSSL using SGX\n");
	
    /*********** SHA-256 ***************/
	if (sha256_print(id) != 0) {
		free_resources(plain, cipher);
		return -1;
	}
	printf("\n");


	/*********** AES-GCM ***************/
	/* place message in first bytes of plain and test encrypt/decrypt with aesgcm */
	memcpy(plain, message, sizeof(message));

	if (aesgcm_encrypt_print(id, plain, cipher, plainSz, (byte*)tag, sizeof(tag)) != 0) {
		free_resources(plain, cipher);
		return -1;
	}

	memset(plain, 0, plainSz);
	if (aesgcm_decrypt_print(id, plain, cipher, plainSz, tag, sizeof(tag)) != 0) {
		free_resources(plain, cipher);
		return -1;
	}

	printf("\tdecrypted message = ");
	for (i = 0; i < sizeof(message); i++) { printf("%c", plain[i]); }
	printf("\n\n");

	/*********** RSA ***************/
	memset(cipher, 0, 256);
	ret = rsa_encrypt_print(id, &sgxStatus, message, sizeof(message), cipher, plainSz);
	if (ret < 0) {
		free_resources(plain, cipher);
		return -1;
	}

	memset(plain, 0, plainSz);
	ret = rsa_decrypt_print(id, plain, plainSz, cipher, sgxStatus);
	if (ret != 0) {
		free_resources(plain, cipher);
		return -1;
	}

	printf("\tdecrypted message = ");
	for (i = 0; i < 15; i++){ printf("%c", plain[i]); }
	printf("\n");


	/*********** Free arrays and exit ***************/
	free_resources(plain, cipher);
	return 0;
}

