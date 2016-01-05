/* wolfsslCertSetup.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
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

#include <stdio.h>

#include <include/x509/wolfsslCert.h>

int wolfsslCertSetup(int argc, char** argv, char action)
{
    int i; /* loop counter */

    for (i = 2; i < argc; i++) {
        if (XSTRNCMP(argv[i], "-help", 5) == 0) {
            wolfsslCertHelp();
            return 0;
        } else if (XSTRNCMP(argv[i], "-h", 2) == 0) {
            wolfsslCertHelp();
            return 0;
        }
    }
    return 0;
}

void wolfsslCertHelp()
{
    printf("\n\n\nThis would be the certificate help.\n\n\n");
}
