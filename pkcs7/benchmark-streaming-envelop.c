/* benchmark-streaming-envelop.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#include <wolfssl/wolfcrypt/logging.h>

#define USE_CERT_BUFFERS_2048
#include <wolfssl/certs_test.h>

#include <stdio.h>
#include <sys/time.h>

#define CONTENT_FILE_NAME "conent-benchmark.bin"
#define ENCODED_FILE_NAME "test-stream-dec.p7b"
#define TEST_STREAM_CHUNK_SIZE 1000

struct timeval startTime;

static void TimeLogStart(void)
{
	gettimeofday(&startTime, NULL);
}


static double ToSeconds(struct timeval* in)
{
    double ret = 0;
    if (in != NULL) {
        ret = (double)(in->tv_sec + (double)(in->tv_usec/1000000.0));
    }
    return ret;
}


static double GetMBs(double dataSz)
{
    struct timeval currentTime;
    double seconds;
    double MBS;

    gettimeofday(&currentTime, NULL);

    seconds = ToSeconds(&currentTime) - ToSeconds(&startTime);
    MBS = dataSz / 1000000.0;

    return MBS/seconds;
}

int CreateContentFile(double contentSz)
{
    FILE* f;
    double i;
    int ret = 0;

    f = fopen(CONTENT_FILE_NAME, "wb");
    if (f == NULL) {
        printf("Unable to create conent file [%s]\n", CONTENT_FILE_NAME);
        ret = -1;
    }
    else {
        for (i = 0; i < contentSz;) {
            double sz = (contentSz - i < 1000)? contentSz -i : 1000;
            byte tmpBuffer[1000];
            int j;

            for (j = 0; j < sz; j++) {
                tmpBuffer[j] = rand() % 256;
            }
            sz = fwrite(tmpBuffer, 1, sz, f);
            if (sz <= 0) {
                printf("Failed to write to content file\n");
                ret = -1;
                break;
            }
            i += sz;
        }
        fclose(f);
    }
    return ret;
}

typedef struct BENCHMARK_IO {
    FILE* in;
    FILE* out;
    byte  buf[TEST_STREAM_CHUNK_SIZE];
} BENCHMARK_IO;


static int GetContentCB(PKCS7* pkcs7, byte** content, void* ctx)
{
    int ret = 0;
    BENCHMARK_IO* io = (BENCHMARK_IO*)ctx;

    if (io != NULL) {
        ret = fread(io->buf, 1, TEST_STREAM_CHUNK_SIZE, io->in);
        if (ret > 0) {
            *content = io->buf;
        }
    }

    (void)pkcs7;
    return ret;
}


static int StreamOutputCB(PKCS7* pkcs7, const byte* output, word32 outputSz,
    void* ctx)
{
    int ret = 0;
    BENCHMARK_IO* io = (BENCHMARK_IO*)ctx;

    if (io != NULL) {
        ret = fwrite(output, 1, outputSz, io->out);
        if (ret < 0) {
            printf("stream output write failed\n");
            ret = -1;
        }
    }

    (void)pkcs7;
    return 0;
}


static int EncodePKCS7Bundle(double contentSz, WC_RNG* rng)
{
    wc_PKCS7* pkcs7;
    double per;
    int ret = 0;
    BENCHMARK_IO io;
    byte aes256Key[] = {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08
    };

    printf("Creating an encoded bundle ... ");
    TimeLogStart();

    pkcs7 = wc_PKCS7_New(NULL, 0);
    if (pkcs7 == NULL) {
        printf("Failed to create PKCS7 struct\n");
        ret = MEMORY_E;
    }

    if (ret == 0) {
        ret = wc_PKCS7_InitWithCert(pkcs7, (byte*)client_cert_der_2048,
            sizeof_client_cert_der_2048);
        if (ret != 0) {
            printf("Failed to init with cert\n");
        }
    }

    if (pkcs7 != NULL) {
    #ifdef ECC_TIMING_RESISTANT
        pkcs7->rng = rng;
    #endif

        pkcs7->content       = NULL; /* pulling content from callback */
        pkcs7->contentSz     = contentSz;
        pkcs7->contentOID    = DATA;
        pkcs7->encryptOID    = AES256CBCb;
        pkcs7->encryptionKey   = aes256Key;
        pkcs7->encryptionKeySz = sizeof(aes256Key);
    }

    /* open the IO files to use */
    if (ret == 0) {
        io.in  = fopen(CONTENT_FILE_NAME, "rb");
        io.out = fopen(ENCODED_FILE_NAME, "wb");
        if (io.in == NULL || io.out == NULL) {
            printf("Failed to open the IO files\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_PKCS7_SetStreamMode(pkcs7, 1, GetContentCB, StreamOutputCB,
            (void*)&io);
        if (ret != 0) {
            printf("Failed to set stream mode\n");
        }
    }

    if (ret == 0) {
        ret = wc_PKCS7_EncodeEnvelopedData(pkcs7, NULL, 0);
        if (ret <= 0) {
            printf("Failed to encode enveloped data\n");
        }
    }
    wc_PKCS7_Free(pkcs7);

    if (ret > 0) {
        per = GetMBs(ret);
        printf("%.2f MB/s", per);
    }
    printf(" : ret = %d\n", ret);
    return 0;
}


static int DecryptCB(wc_PKCS7* pkcs7,
    const byte* output, word32 outputSz, void* ctx) {
     FILE* out = (FILE*)ctx;

    if (out == NULL) {
        return -1;
    }

    /* printf("Decoded in %d bytes\n", outputSz);
     * for (word32 z = 0; z < outputSz; z++) printf("%02X", output[z]);
     * printf("\n");
    */
    fwrite(output, 1, outputSz, out);

    (void)pkcs7;
    return 0;
}

static int DecodePKCS7Bundle(void)
{
    wc_PKCS7* pkcs7 = NULL;
    double per;
    int ret = 0;
    FILE* f = NULL;
    FILE* out = NULL;
    double totalSz = 0;
    byte testStreamBuffer[TEST_STREAM_CHUNK_SIZE];
    int testStreamBufferSz = 0;

    printf("Decoding PKCS7 bundle ... ");
    TimeLogStart();


    pkcs7 = wc_PKCS7_New(NULL, 0);
    if (pkcs7 == NULL) {
        ret = MEMORY_E;
    }

    if (ret == 0) {
        ret = wc_PKCS7_InitWithCert(pkcs7, (byte*)client_cert_der_2048,
            sizeof_client_cert_der_2048);
    }

    if (ret == 0) {
        ret = wc_PKCS7_SetKey(pkcs7, (byte*)client_key_der_2048,
            sizeof_client_key_der_2048);
    }

    if (ret == 0) {
        out = fopen("benchmark-decrypted.bin", "wb");
        if (out == NULL) {
            printf("Unable to open decrypted data out file\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        ret = wc_PKCS7_SetStreamMode(pkcs7, 1, NULL, DecryptCB, (void*)out);
    }

    if (ret == 0) {
        f = fopen(ENCODED_FILE_NAME, "rb");
        if (f == NULL) {
            printf("Unable to open encoded file\n");
            ret = -1;
        }
    }

    if (ret == 0) {
        do {
            testStreamBufferSz = (int)XFREAD(testStreamBuffer, 1,
                sizeof(testStreamBuffer), f);

            ret = wc_PKCS7_DecodeEnvelopedData(pkcs7, testStreamBuffer,
                testStreamBufferSz, NULL, 0);
            totalSz += testStreamBufferSz;
        } while (ret == WC_PKCS7_WANT_READ_E);
    }

    /* success with decoding */
    if (ret >= 0) {
        ret = 0;
    }

    if (f != NULL) {
        fclose(f);
    }
    if (out != NULL) {
        fclose(out);
    }

    wc_PKCS7_Free(pkcs7);

    if (ret == 0) {
        per = GetMBs(totalSz);
        printf("%.2f MB/s", per);
    }
    printf(" : ret = %d\n", ret);
    return ret;
}


int main(int argc, char** argv)
{
    double contentSz = 10000;
    WC_RNG rng;
    int ret;

    if (argc > 1) {
        contentSz = atof(argv[1]);
    }

    ret = wolfCrypt_Init();
    if (ret != 0) {
        printf("Failed to init wolfCrypt\n");
    }
    wolfSSL_Debugging_ON();

    if (ret == 0) {
        ret = wc_InitRng(&rng);
    }

    if (ret == 0) {
        ret = CreateContentFile(contentSz);
    }

    if (ret == 0) {
        ret = EncodePKCS7Bundle(contentSz, &rng);
    }

    if (ret == 0) {
        ret = DecodePKCS7Bundle();
    }

    wc_FreeRng(&rng);

    wolfCrypt_Cleanup();
    return 0;
}

