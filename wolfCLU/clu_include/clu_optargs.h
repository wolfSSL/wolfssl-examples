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
    /* @temporary: implement modes as arguments */
    ENCRYPT = 1000,
    DECRYPT,
    BENCHMARK,
    HASH,
    X509,
    REQUEST,
    GEN_KEY,

    INFILE,
    OUTFILE,
    PASSWORD,
    KEY,
    IV,
    ALL,
    SIZE,
    EXPONENT,
    TIME,
    VERIFY,
    VERBOSE,
    INFORM,
    OUTFORM,
    NOOUT,
    TEXT_OUT,
    SILENT,
    OUTPUT,
    HELP,
};

/* Structure for holding long arguments */
static struct option long_options[] = {

    /* @temporary: implement modes as flags */
    {"encrypt", optional_argument, 0, ENCRYPT   },
    {"decrypt", optional_argument, 0, DECRYPT   },
    {"bench",   optional_argument, 0, BENCHMARK },
    {"hash",    optional_argument, 0, HASH      },
    {"x509",    no_argument,       0, X509      },
    {"req",     optional_argument, 0, REQUEST   },
    {"genkey",  optional_argument, 0, GEN_KEY   },

    {"in",      required_argument, 0, INFILE    },
    {"out",     required_argument, 0, OUTFILE   },
    {"pwd",     required_argument, 0, PASSWORD  },
    {"key",     required_argument, 0, KEY       },
    {"iv",      required_argument, 0, IV        },
    {"all",     0,                 0, ALL       },
    {"size",    required_argument, 0, SIZE      },
    {"exponent",required_argument, 0, EXPONENT  },
    {"time",    required_argument, 0, TIME      },
    {"verify",  0,                 0, VERIFY    },
    {"verbose", 0,                 0, VERBOSE   },
    {"inform",  required_argument, 0, INFORM    },
    {"outform", required_argument, 0, OUTFORM   },
    {"noout",   0,                 0, NOOUT     },
    {"text",    0,                 0, TEXT_OUT  },
    {"silent",  0,                 0, SILENT    },
    {"output",  0,                 0, OUTPUT    },
    {"help",    0,                 0, HELP      },
    {"h",       0,                 0, HELP      },
    {"v",       0,                 0, 'v'       },
    {"version", 0,                 0, 'v'       },

    {0, 0, 0, 0} /* terminal element */
};

/* method for converting arguments to lower case */
void convert_to_lower(char* s, int sSz);

