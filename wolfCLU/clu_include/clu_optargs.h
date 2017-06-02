/* clu_optargs.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
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


/* Enumerated types for long arguments */
enum {
    ENCRYPT = 1000,
    DECRYPT,
    BENCHMARK,
    HASH,
    INFILE,
    OUTFILE,
    PASSWORD,
    KEY,
    IV,
    ALL,
    SIZE,
    TIME,
    VERIFY,
    VERBOSE,
    X509,
    INFORM,
    OUTFORM,
    NOOUT,
    TEXT_OUT,
    SILENT,
    REQUEST,
    GEN_KEY,
    OUTPUT,
    HELP1, /* "-help" */
    HELP2, /* "-h" */
};

/* Structure for holding long arguments */
static struct option long_options[] = {

    {"encrypt", required_argument, 0, ENCRYPT   },
    {"decrypt", required_argument, 0, DECRYPT   },
    {"bench",   required_argument, 0, BENCHMARK },
    {"hash",    required_argument, 0, HASH      },
    {"in",      required_argument, 0, INFILE    },
    {"out",     required_argument, 0, OUTFILE   },
    {"pwd",     required_argument, 0, PASSWORD  },
    {"key",     required_argument, 0, KEY       },
    {"iv",      required_argument, 0, IV        },
    {"all",     0,                 0, ALL       },
    {"size",    required_argument, 0, SIZE      },
    {"time",    required_argument, 0, TIME      },
    {"verify",  0,                 0, VERIFY    },
    {"verbose", 0,                 0, VERBOSE   },
    {"x509",    0,                 0, X509      },
    {"inform",  required_argument, 0, INFORM    },
    {"outform", required_argument, 0, OUTFORM   },
    {"noout",   0,                 0, NOOUT     },
    {"text",    0,                 0, TEXT_OUT  },
    {"silent",  0,                 0, SILENT    },
    {"req",     0,                 0, REQUEST   },
    {"genkey",  0,                 0, GEN_KEY   },
    {"output",  0,                 0, OUTPUT    },
    {"help",    0,                 0, HELP1     },
    {"h",       0,                 0, HELP2     },
    {"v",       0,                 0, 'v'       },
    {"version", 0,                 0, 'v'       },
    {0, 0, 0, 0}

};

/* method for converting arguments to lower case */
void convert_to_lower(char* s, int sSz);

