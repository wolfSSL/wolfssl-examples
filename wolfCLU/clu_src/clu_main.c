/* clu_main.c
 *
 * Copyright (C) 2006-2016 wolfSSL Inc.
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

#include "clu_include/clu_header_main.h"
#include "clu_include/x509/clu_cert.h"
#include "clu_include/clu_optargs.h"
#include "clu_include/clu_error_codes.h"
/* enumerate optionals beyond ascii range to dis-allow use of alias IE we
 * do not want "-e" to work for encrypt, user must use "encrypt"
 */

int main(int argc, char** argv)
{
    int ret = 0, option = 0, long_index = 0;
    int i;

    if (argc == 1) {
        printf("Main Help.\n");
        wolfsslHelp();
    }

    /* flexibility: allow users to input any CAPS or lower case,
     * we will do all processing on lower case only. */
    for (i = 0; i < argc; i++) {
        convert_to_lower(argv[i], (int) XSTRLEN(argv[i]));
    }

    while ((option = getopt_long_only(argc, argv,"",
                   long_options, &long_index )) != -1) {


        switch (option) {
            /* Encrypt */
            case ENCRYPT:  ret = wolfsslSetup(argc, argv, 'e');
                            break;
            /* Decrypt */
            case DECRYPT:  ret = wolfsslSetup(argc, argv, 'd');;
                            break;
            /* Benchmark */
            case BENCHMARK:ret = wolfsslBenchSetup(argc, argv);
                            break;
            /* Hash */
            case HASH:     ret = wolfsslHashSetup(argc, argv);
                            break;
            /* x509 Certificate processing */
            case X509:     ret = wolfsslCertSetup(argc, argv);
                            break;

/* Ignore the following arguments for now. Will be handled by their respective
 * setups IE Crypto setup, Benchmark setup, or Hash Setup */

            /* File passed in by user */
            case INFILE:    break;
            /* Output file */
            case OUTFILE:   break;
            /* Password */
            case PASSWORD:  break;
            /* Key if used must be in hex */
            case KEY:       break;
            /* IV if used must be in hex */
            case IV:        break;
            /* Opt to benchmark all available algorithms */
            case ALL:       break;
            /* size for hash to output */
            case SIZE:      break;
            /* Time to benchmark for 1-10 seconds optional default: 3s */
            case TIME:      break;
            /* Verify results, used with -iv and -key */
            case VERIFY:    break;
            /* Certificate Stuff*/
            case INFORM:    break;
            case OUTFORM:   break;
            /* which version of clu am I using */
            case VERBOSE:
                            wolfsslVerboseHelp();
                            return 0;
/*End of ignored arguments */

            case 'v':       wolfsslVersion();
                            return 0;

             default:
                            printf("Main help default.\n");
                            wolfsslHelp();
                            return 0;
        }
    }

    if (ret != 0)
        printf("Error returned: %d.\n", ret);

    return ret;
}

void convert_to_lower(char* s, int sSz)
{
    int i;
    for (i = 0; i < sSz; i++) {
        s[i] = tolower(s[i]);
    }
}

