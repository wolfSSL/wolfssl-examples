/* wolfsslMain.c
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

#include "include/wolfssl.h"

int main(int argc, char** argv)
{
    int ret = 0;
    int option;
    if (argc == 1) {
        printf("Main Help.\n");
        wolfsslHelp();
    }
    while ((option = getopt (argc, argv, "e:d:h:b:i:o:p:V:K:t:axv")) != -1)  {
        switch (option)
        {
            /* User wants to encrypt data or file*/
            case 'e':
                if (argc == 2) {
                    wolfsslEncryptHelp();
                    return 0;
                }
                ret = wolfsslSetup(argc, argv, 'e');
                return 0;
                /* User wants to decrypt some data or file */
            case 'd':
                ret = wolfsslSetup(argc, argv, 'd');
                return 0;
                /* User wants to hash some data/file */
            case 'h':
                ret = wolfsslHashSetup(argc, argv);
                return 0;
            case 'b':
                ret = wolfsslBenchSetup(argc, argv);
                return 0;
            case 'i':/* will be handled by Setup function */
                break;
            case 'o':/* will be handled by Setup function */
                break;
            case 'p':/* will be handled by Setup function */
                break;
            case 'V':/* will be handled by Setup function */
                break;
            case 'K':/* will be handled by Setup function */
                break;
            case 'a':/* will be handled by benchmarkSetup function */
                break;
            case 't':/* will be handled by benchSetup function */
                break;
            case 'x':/* will be handled by benchSetup function*/
                break;
            case 'v':
                wolfsslVersion();
                return 0;
                break;
            default:
                printf("Main Help Default.\n");
                wolfsslVerboseHelp();
                return 0;
        }
    }
    if (ret != 0)
        printf("Error returned: %d.\n", ret);
    return ret;
}
