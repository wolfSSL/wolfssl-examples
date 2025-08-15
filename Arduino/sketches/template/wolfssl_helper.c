/* my_library.cpp
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/* This is a sample include directory library using wolfSSL.
 *
 * Do not explicitly include wolfSSL user_settings.h here.
 *
 * Be sure to include these files in all libraries that reference
 * wolfssl in this order: */

#if defined(ARDUINO_PORTENTA_X8)
    /* This file is purposely a c and not .cpp file for testing.
     * On Portenta X8 the core headers assume C++, and things like A6,
     * PIN_SPI_MOSI, etc. - rely on C++-only constructs.
     * So don't include Arduino.h here for Portenta. */

    #include <wolfssl/wolfcrypt/settings.h>
    #include <wolfssl/ssl.h> /* The ssl.h usually included by wolfssl.h */

    #ifdef __cplusplus
    extern "C" {
    #endif

    /* Sample source code is C, but Arduino is compiling with C++
     * Declare a helper function to be used in wolfssl/wolfcrypt/logging.c */
    int wolfSSL_Arduino_Serial_Print(const char* const s);

    #ifdef __cplusplus
    }
    #endif
#else
    /* Assume all other target boards would want to include Arduino.h in a
     * helper such as this one. Not needed in this wolfssl_helper.c example. */
    #include <Arduino.h>

     /* settings.h is typically included in wolfssl.h, but here as a reminder: */
    #include <wolfssl/wolfcrypt/settings.h>
    #include <wolfssl.h> /* The wolfssl core Arduino library file */
#endif


#include "wolfssl_helper.h"

int wolfssl_helper_sample()
{
    /* We cannot use Serial.print in a "c" file */
    /* Serial.print("Hello world!"); */
    int ret;
    printf("Hello wolfssl_helper_sample!\r\n");

    printf("- Calling wolfSSL_Init()\r\n");
    ret = wolfSSL_Init();
    if (ret == WOLFSSL_SUCCESS) {
        printf("- Success wolfssl_helper!\r\n");
    }
    else {
        printf("- Error initializing wolfSSL!\r\n");
    }
    return ret;
}
