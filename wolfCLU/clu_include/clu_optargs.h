/* clu_optargs.h
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
    PKEY,
    RSA,
    ECC,
    ED25519,

    CERT_SHA,
    CERT_SHA224,
    CERT_SHA256,
    CERT_SHA384,
    CERT_SHA512,

    INFILE,
    OUTFILE,
    PASSWORD,
    KEY,
    IV,
    NEW,
    ALL,
    SIZE,
    EXPONENT,
    TIME,
    SIGN,
    VERIFY,
    VERBOSE,
    INKEY,
    PUBIN,
    PUBOUT,
    PUBKEY,
    SIGFILE,
    INFORM,
    OUTFORM,
    NOOUT,
    TEXT_OUT,
    SILENT,
    OUTPUT,
    HELP,
};

/* Structure for holding modes for long arguments. These are the modes that then
 * will take in any additional arguments. An example of a mode would be x509
 * in the command 'wolfssl x509 -in cert.pem -text' */
static struct option mode_options[] = {
    {"encrypt",   required_argument, 0, ENCRYPT   },
    {"decrypt",   required_argument, 0, DECRYPT   },
    {"bench",     no_argument,       0, BENCHMARK },
    {"hash",      required_argument, 0, HASH      },
    {"x509",      no_argument,       0, X509      },
    {"req",       no_argument,       0, REQUEST   },
    {"genkey",    required_argument, 0, GEN_KEY   },
    {"pkey",      no_argument,       0, PKEY      },
    {"rsa",       no_argument,       0, RSA       },
    {"ecc",       no_argument,       0, ECC       },
    {"ed25519",   no_argument,       0, ED25519   },
    {"help",      no_argument,       0, HELP      },
    {"h",         no_argument,       0, HELP      },
    {"v",         no_argument,       0, 'v'       },
    {"version",   no_argument,       0, 'v'       },

    {0, 0, 0, 0} /* terminal element */
};


/* Structure for holding long arguments. These are arguments passed into the
 * modes. An example of an argument would be '-out' in the following command
 * 'wolfssl x509 -out cert.pem' */
static struct option long_options[] = {

    {"sha",       no_argument,       0, CERT_SHA   },
    {"sha224",    no_argument,       0, CERT_SHA224},
    {"sha256",    no_argument,       0, CERT_SHA256},
    {"sha384",    no_argument,       0, CERT_SHA384},
    {"sha512",    no_argument,       0, CERT_SHA512},

    {"in",        required_argument, 0, INFILE    },
    {"out",       required_argument, 0, OUTFILE   },
    {"pwd",       required_argument, 0, PASSWORD  },
    {"key",       required_argument, 0, KEY       },
    {"new",       no_argument,       0, NEW       },
    {"iv",        required_argument, 0, IV        },
    {"all",       no_argument,       0, ALL       },
    {"size",      required_argument, 0, SIZE      },
    {"exponent",  required_argument, 0, EXPONENT  },
    {"time",      required_argument, 0, TIME      },
    {"sign",      no_argument,       0, SIGN      },
    {"verify",    no_argument,       0, VERIFY    },
    {"verbose",   no_argument,       0, VERBOSE   },
    {"inkey",     required_argument, 0, INKEY     },
    {"pubin",     no_argument,       0, PUBIN     },
    {"pubout",    no_argument,       0, PUBOUT    },
    {"pubkey",    no_argument,       0, PUBKEY    },
    {"inform",    required_argument, 0, INFORM    },
    {"outform",   required_argument, 0, OUTFORM   },
    {"sigfile",   required_argument, 0, SIGFILE   },
    {"noout",     no_argument,       0, NOOUT     },
    {"text",      no_argument,       0, TEXT_OUT  },
    {"silent",    no_argument,       0, SILENT    },
    {"output",    required_argument, 0, OUTPUT    },

    {0, 0, 0, 0} /* terminal element */
};

/* method for converting arguments to lower case */
void convert_to_lower(char* s, int sSz);

