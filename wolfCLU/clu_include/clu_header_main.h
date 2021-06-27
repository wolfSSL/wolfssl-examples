/* clu_header_main.h
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#ifndef _WOLFSSL_CLU_HEADER_
#define _WOLFSSL_CLU_HEADER_

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <getopt.h>

/* wolfssl includes */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/des3.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/version.h>
#include <wolfssl/openssl/bio.h>

#ifndef NO_MD5
    #include <wolfssl/wolfcrypt/md5.h>
#endif

#ifndef NO_SHA
    #include <wolfssl/wolfcrypt/sha.h>
#endif

#ifndef NO_SHA256
    #include <wolfssl/wolfcrypt/sha256.h>
#endif

#ifdef WOLFSSL_SHA512
    #include <wolfssl/wolfcrypt/sha512.h>
#endif

#ifdef HAVE_BLAKE2
    #include <wolfssl/wolfcrypt/blake2.h>
#endif

#ifdef HAVE_CAMELLIA
    #include <wolfssl/wolfcrypt/camellia.h>
#endif

#include <wolfssl/wolfcrypt/coding.h>

#define BLOCK_SIZE 16384
#define MEGABYTE (1024*1024)
#define MAX_THREADS 64
#define MAX_FILENAME_SZ 256
#define CLU_4K_TYPE 4096
#if LIBWOLFSSL_VERSION_HEX >= 50413568 /* int val of hex 0x0301400 = 50413568 */
    #define CLU_SHA256 WC_SHA256
#else
    #define CLU_SHA256 SHA256
#endif

#include <wolfssl/wolfcrypt/types.h>

#ifdef WOLFSSL_STATIC_MEMORY
    #include <wolfssl/wolfcrypt/memory.h>
    static WOLFSSL_HEAP_HINT* HEAP_HINT;
#else
    #define HEAP_HINT NULL
#endif

#include "clu_include/clu_error_codes.h"

 /* @VERSION
  * Update every time library change,
  * functionality shift,
  * or code update
  */
#define VERSION 0.3

/* encryption argument function
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 * @param action forwarded from wolfCLU_main (-e, -d, -h, or -b)
 */
int wolfCLU_setup(int argc, char** argv, char action);


/* Handle ecparam mode
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 * @return returns 0 on success
 */
int wolfCLU_ecparam(int argc, char** argv);

/* hash argument function
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 */
int wolfCLU_hashSetup(int argc, char** argv);

/* benchmark argument function
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 */
int wolfCLU_benchSetup(int argc, char** argv);

/*
 * generic help function
 */
void wolfCLU_help(void);

/*
 * verbose help function
 */
void wolfCLU_verboseHelp(void);

/*
 * encrypt help function
 */
void wolfCLU_encryptHelp(void);

/*
 * decrypt help function
 */
void wolfCLU_decryptHelp(void);

/*
 * hash help function
 */
void wolfCLU_hashHelp(void);

/*
 * benchmark help function
 */
void wolfCLU_benchHelp(void);

/*
 * genkey help function
 */
void wolfCLU_genKeyHelp(void);

/*
 * sign help function
 */
void wolfCLU_signHelp(int);

/*
 * verify help function
 */
void wolfCLU_verifyHelp(int);

/*
 * certgen help function
 */
void wolfCLU_certgenHelp();


/* find algorithm for encryption/decryption
 * 
 * @param name the whole line sent from user. Example: "aes-cbc-128"
 * @param alg the algorithm specified by the user (aes, 3des, or camellia)
 * @param mode the mode as set by the user (cbc or ctr)
 * @param size set based on the algorithm specified
 */
int wolfCLU_getAlgo(int argc, char* argv[], char** alg, char** mode, int* size);

/* secure entry of password
 *
 * @param pwdKey the password provide by the user
 * @param size the size as determnined by wolfCLU_GetAlgo
 */
int wolfCLU_noEcho(char* pwdKey, int size);

/* adds characters to end of string
 *
 * @param s the char array we'll be appending to
 * @param c the char that will be appended to s
 */
void wolfCLU_append(char* s, char c);

/* interrupt function
 *
 * @param signo gets type cast to void, interrupts the loop.
 */
void wolfCLU_stop(int signo);

/* finds current time during runtime */
double wolfCLU_getTime(void);

/* A function to convert from Hex to Binary
 *
 * @param h1 a char array containing hex values to be converted, can be NULL
 * @param h2 a char array containing hex values to be converted, can be NULL
 * @param h3 a char array containing hex values to be converted, can be NULL
 * @param h4 a char array containing hex values to be converted, can be NULL
 * @param b1 a buffer to store the result of h1 conversion, can be NULL
 * @param b2 a buffer to store the result of h2 conversion, can be NULL
 * @param b3 a buffer to store the result of h3 conversion, can be NULL
 * @param b4 a buffer to store the result of h4 conversion, can be NULL
 * @param b1Sz a word32 that will be set after conversion of b1, can be NULL
 * @param b2Sz a word32 that will be set after conversion of b2, can be NULL
 * @param b3Sz a word32 that will be set after conversion of b3, can be NULL
 * @param b4Sz a word32 that will be set after conversion of b4, can be NULL
 */
int wolfCLU_hexToBin(const char* h1, byte** b1, word32* b1Sz,
                    const char* h2, byte** b2, word32* b2Sz,
                    const char* h3, byte** b3, word32* b3Sz,
                    const char* h4, byte** b4, word32* b4Sz);

/* A function to free MALLOCED buffers
 *
 * @param b1 a buffer to be freed, can be set to NULL
 * @param b2 a buffer to be freed, can be set to NULL
 * @param b3 a buffer to be freed, can be set to NULL
 * @param b4 a buffer to be freed, can be set to NULL
 * @param b5 a buffer to be freed, can be set to NULL
 */
void wolfCLU_freeBins(byte* b1, byte* b2, byte* b3, byte* b4, byte* b5);

/* function to display stats results from benchmark
 *
 * @param start the time when the benchmark was started
 * @param blockSize the block size of the algorithm being benchmarked
 */
void wolfCLU_stats(double start, int blockSize, int64_t blocks);

/* encryption function
 *
 * @param alg this will be the algorithm to use as specified by the user
 *        options include: aes, 3des, or camellia
 * @param mode this is the mode to be used for the encryption
 *        cbc is used with all of the above with an optional ctr for aes
 * @param pwdKey this is the user provided password to be used as the key
 * @param key if entered must be in hex, can be used to verify encryption with
 *            nist test vectors.
 * @param size this is set by wolfCLU_GetAlgo and is used to stretch the
 *        password
 * @param in the filename or user input from command line
 * @param out the filename to output following en/de cryption
 * @param iv if entered must be in hex otherwise generated at run time
 * @param block size of block as determined by the algorithm being used
 * @param ivCheck a flag if user inputs a specific IV
 * @param inputHex a flag to specify encrypting hex data, instead of byte data
 */
int wolfCLU_encrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size,
                                char* in, char* out, byte* iv, int block,
                                int ivCheck, int inputHex);

/* decryption function
 *
 * @param alg this will be the algorithm to use as specified by the user
 *        options include: aes, 3des, or camellia
 * @param mode this is the mode to be used for the encryption
 *        cbc is used with all of the above with an optional ctr for aes
 * @param pwdKey this is the user provided password to be used as the key
 * @param key if entered must be in hex, can be used to verify encryption with
 *            nist test vectors.
 * @param size this is set by wolfCLU_GetAlgo and is used to stretch the
 *        password
 * @param in the filename or user input from command line
 * @param out the filename to output following en/de cryption
 * @param iv if entered must be in hex otherwise generated at run time
 * @param block size of block as determined by the algorithm being used
 * @param keyType let's decrypt know if it's using a password based key or a
 *        hexidecimal, user specified key.
 */
int wolfCLU_decrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size,
                    char* in, char* out, byte* iv, int block, int keyType);

/* benchmarking function
 *
 * @param timer a timer to be started and stopped for benchmarking purposes
 * @param option a flag to allow benchmark execution
 */
int wolfCLU_benchmark(int timer, int* option);

/* hashing function
 *
 * @param in
 * @param len
 * @param out
 * @param alg
 * @param size
 */
int wolfCLU_hash(WOLFSSL_BIO* bioIn, WOLFSSL_BIO* bioOut, char* alg, int size);

int wolfCLU_md5Setup(int argc, char** argv);

/*
 * get the current Version
 */
int wolfCLU_version(void);

/*
 * generic function to check for a specific input argument. Return the
 * argv[i] where argument was found. Useful for getting following value after
 * arg.
 * EXAMPLE:
 * --------------------
 * int ret;
 * char myString[BIG_ENOUGH_FOR_INPUT];
 * ret = wolfCLU_checkForArg("-somearg");
 * if (ret > 0)
 *     XSTRNCPY(myString, argv[ret+1], XSTRLEN(argv[ret+1]));
 * else {
 *      <ERROR LOGIC>
 * }
 * --------------------
 *
 *
 */
int wolfCLU_checkForArg(char* searchTerm, int length, int argc, char** argv);

/*
 * Verify valid output format
 */
int wolfCLU_checkOutform(char* outform);

/*
 * Verify valid input format
 */
int wolfCLU_checkInform(char* inform);


/**
 *  @ingroup X509
 *  @brief This function is used internally to get user input and fill out a
 *  WOLFSSL_X509_NAME structure.
 *
 *  @param x509 the name structure to be filled in
 *  @return 0 On successfully setting the name
 */
int wolfCLU_CreateX509Name(WOLFSSL_X509_NAME* x509);


/**
 * @ingroup X509
 * @brief This function reads a configure file and creates the resulting
 *  WOLFSSL_X509 structure
 *
 * @param config file name of the config to read
 * @param sect   section in the config file to search for when reading
 * @return a newly created WOLFSSL_X509 structure on success
 * @return null on fail
*/
WOLFSSL_X509* wolfCLU_readConfig(char* config, char* sect);

#endif /* _WOLFSSL_CLU_HEADER_ */
