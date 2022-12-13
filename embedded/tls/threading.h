/* threading.h
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

#ifndef SINGLE_THREADED

#if defined(_POSIX_THREADS) && !defined(__MINGW32__)
    typedef void*         THREAD_RETURN;
    typedef pthread_t     THREAD_TYPE;
    #define WOLFSSL_THREAD
    #define INFINITE -1
    #define WAIT_OBJECT_0 0L
#elif defined(WOLFSSL_MDK_ARM)|| defined(WOLFSSL_KEIL_TCP_NET)
    typedef unsigned int  THREAD_RETURN;
    typedef int           THREAD_TYPE;
    #define WOLFSSL_THREAD
#elif defined(WOLFSSL_TIRTOS)
    typedef void          THREAD_RETURN;
    typedef Task_Handle   THREAD_TYPE;
    #define WOLFSSL_THREAD
#elif defined(WOLFSSL_ZEPHYR)
    typedef void            THREAD_RETURN;
    typedef struct k_thread THREAD_TYPE;
    #define WOLFSSL_THREAD
#elif defined(USE_WINDOWS_API)
    typedef unsigned int  THREAD_RETURN;
    typedef intptr_t      THREAD_TYPE;
    #define WOLFSSL_THREAD __stdcall
#else
    #error "Threading API not supported in sample"
#endif

typedef THREAD_RETURN WOLFSSL_THREAD THREAD_FUNC(void*);
typedef struct func_args {
    int    argc;
    char** argv;
    int    return_code;
} func_args;


static void start_thread(THREAD_FUNC, func_args*, THREAD_TYPE*);
static void join_thread(THREAD_TYPE);

#endif

