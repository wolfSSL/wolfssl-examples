/* util.h
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,USA
 */

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>

/* cyassl includes */
#include <cyassl/options.h>
#include <cyassl/ctaocrypt/random.h>
#include <cyassl/ctaocrypt/pwdbased.h>
#include <cyassl/ctaocrypt/aes.h>
#include <cyassl/ctaocrypt/des3.h>
#include <cyassl/ctaocrypt/error-crypt.h>
#include <cyassl/error-ssl.h>

#ifndef NO_MD5
    #include <cyassl/ctaocrypt/md5.h>
#endif

#ifndef NO_SHA
    #include <cyassl/ctaocrypt/sha.h>
#endif

#ifndef NO_SHA256
    #include <cyassl/ctaocrypt/sha256.h>
#endif

#ifdef CYASSL_SHA512
    #include <cyassl/ctaocrypt/sha512.h>
#endif

#ifdef HAVE_BLAKE2
    #include <cyassl/ctaocrypt/blake2.h>
#endif

#ifdef HAVE_CAMELLIA
    #include <cyassl/ctaocrypt/camellia.h>
#endif

#ifndef UTIL_H_INCLUDED
	#define UTIL_H_INCLUDED

#define BLOCK_SIZE 16384
#define MEGABYTE (1024*1024)
#define MAX_THREADS 64

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
 * @param action forwarded from wolfsslMain (-e, -d, -h, or -b)
 */
int wolfsslSetup(int argc, char** argv, char action);

/* hash argument function
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 */
int wolfsslHashSetup(int argc, char** argv);

/* benchmark argument function
 *
 * @param argc holds all command line input
 * @param argv each holds one value from the command line input
 */
int wolfsslBenchSetup(int argc, char** argv);

/* 
 * generic help function 
 */
void wolfsslHelp(void);

/* 
 * verbose help function 
 */
void wolfsslVerboseHelp(void);

/* 
 * encrypt help function 
 */
void wolfsslEncryptHelp(void);

/* 
 * decrypt help function
 */
void wolfsslDecryptHelp(void);

/* 
 * hash help function 
 */
void wolfsslHashHelp(void);

/* 
 * benchmark help function
 */
void wolfsslBenchHelp(void);

/* find algorithm for encryption/decryption 
 * 
 * @param name the whole line sent from user. Example: "aes-cbc-128"
 * @param alg the algorithm specified by the user (aes, 3des, or camellia)
 * @param mode the mode as set by the user (cbc or ctr)
 * @param size set based on the algorithm specified
 */
int wolfsslGetAlgo(char* name, char** alg, char** mode, int* size);

/* generates key based on password provided 
 * 
 * @param rng the random number generator
 * @param pwdKey the password based key as provided by the user
 * @param size size as determined by wolfsslGetAlgo
 * @param salt the buffer to store the resulting salt after it's generated
 * @param pad a flag to let us know if there are padded bytes or not
 */
int wolfsslGenKey(RNG* rng, byte* pwdKey, int size, byte* salt, int pad);

/* secure entry of password 
 *
 * @param pwdKey the password provide by the user
 * @param size the size as determnined by wolfsslGetAlgo
 */
int wolfsslNoEcho(char* pwdKey, int size);

/* adds characters to end of string
 *
 * @param s the char array we'll be appending to
 * @param c the char that will be appended to s
 */
void wolfsslAppend(char* s, char c);

/* interrupt function
 *
 * @param signo gets type cast to void, interrupts the loop.
 */
void wolfsslStop(int signo);

/* finds current time during runtime */
double wolfsslGetTime(void);

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
int wolfsslHexToBin(const char* h1, byte** b1, word32* b1Sz,
                    const char* h2, byte** b2, word32* b2Sz,
                    const char* h3, byte** b3, word32* b3Sz,
                    const char* h4, byte** b4, word32* b4Sz);

/* A function to free malloced byte* buffers after conversion
 *
 * @param b1 a buffer to be freed, can be set to NULL
 * @param b2 a buffer to be freed, can be set to NULL
 * @param b3 a buffer to be freed, can be set to NULL
 * @param b4 a buffer to be freed, can be set to NULL
 * @param b5 a buffer to be freed, can be set to NULL
 */
void wolfsslFreeBins(byte* b1, byte* b2, byte* b3, byte* b4, byte* b5);

/* function to display stats results from benchmark
 *
 * @param start the time when the benchmark was started
 * @param blockSize the block size of the algorithm being benchmarked
 */
void wolfsslStats(double start, int blockSize, int64_t blocks);

/* encryption function
 *
 * @param alg this will be the algorithm to use as specified by the user
 *        options include: aes, 3des, or camellia
 * @param mode this is the mode to be used for the encryption
 *        cbc is used with all of the above with an optional ctr for aes
 * @param pwdKey this is the user provided password to be used as the key
 * @param key if entered must be in hex, can be used to verify encryption with
 *            nist test vectors.
 * @param size this is set by wolfsslGetAlgo and is used to stretch the password
 * @param in the filename or user input from command line
 * @param out the filename to output following en/de cryption
 * @param iv if entered must be in hex otherwise generated at run time
 * @param block size of block as determined by the algorithm being used
 * @param ivCheck a flag if user inputs a specific IV
 * @param inputHex a flag to specify encrypting hex data, instead of byte data
 */
int wolfsslEncrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size, 
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
 * @param size this is set by wolfsslGetAlgo and is used to stretch the password
 * @param in the filename or user input from command line
 * @param out the filename to output following en/de cryption
 * @param iv if entered must be in hex otherwise generated at run time
 * @param block size of block as determined by the algorithm being used
 * @param keyType let's decrypt know if it's using a password based key or a 
 *        hexidecimal, user specified key.
 */
int wolfsslDecrypt(char* alg, char* mode, byte* pwdKey, byte* key, int size, 
						char* in, char* out, byte* iv, int block, int keyType);

/* benchmarking function 
 *
 * @param timer a timer to be started and stopped for benchmarking purposes
 * @param option a flag to allow benchmark execution
 */
int wolfsslBenchmark(int timer, int* option);

/* hashing function 
 *
 * @param in 
 * @param len
 * @param out
 * @param alg
 * @param size
 */
int wolfsslHash(char* in, char* out, char* alg, int size);
/*
 * get the current Version
 */
void wolfsslVersion(void);
#endif


