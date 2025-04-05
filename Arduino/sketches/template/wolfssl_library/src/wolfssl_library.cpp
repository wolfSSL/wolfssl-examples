/* wolfssl_library.cpp
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
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
#include <Arduino.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl.h>

/* Arduino source is typically in a `src` directory, with header in parent: */
#include "../wolfssl_library.h"

int wolfssl_library_sample()
{
    int ret = 0;
    Serial.println("\nHello wolfssl_library!");

    printf("- Calling wolfSSL_Init()\r\n");
    ret = wolfSSL_Init();
    if (ret == WOLFSSL_SUCCESS) {
        printf("- Success wolfssl_library!\r\n");
    }
    else {
        printf("- Error initializing wolfSSL!\r\n");
    }
    return ret;
}
