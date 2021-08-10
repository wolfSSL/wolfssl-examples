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


#ifndef WOLFCLU_OPTARGS_H
#define WOLFCLU_OPTARGS_H

/* Enumerated types for long arguments */
enum {
    /* @temporary: implement modes as arguments */
    WOLFCLU_ENCRYPT = 1000,
    WOLFCLU_DECRYPT,
    WOLFCLU_CRYPT,
    WOLFCLU_BENCHMARK,
    WOLFCLU_HASH,
    WOLFCLU_MD,
    WOLFCLU_X509,
    WOLFCLU_REQUEST,
    WOLFCLU_GEN_KEY,
    WOLFCLU_ECPARAM,
    WOLFCLU_PKEY,

    WOLFCLU_INFILE,
    WOLFCLU_OUTFILE,
    WOLFCLU_PASSWORD,
    WOLFCLU_KEY,
    WOLFCLU_IV,
    WOLFCLU_NEW,
    WOLFCLU_ALL,
    WOLFCLU_SIZE,
    WOLFCLU_EXPONENT,
    WOLFCLU_TIME,
    WOLFCLU_SIGN,
    WOLFCLU_VERIFY,
    WOLFCLU_DGST,
    WOLFCLU_VERBOSE,
    WOLFCLU_INKEY,
    WOLFCLU_PUBIN,
    WOLFCLU_PUBOUT,
    WOLFCLU_PUBKEY,
    WOLFCLU_SIGFILE,
    WOLFCLU_CONFIG,
    WOLFCLU_CURVE_NAME,
    WOLFCLU_DAYS,
    WOLFCLU_INFORM,
    WOLFCLU_OUTFORM,
    WOLFCLU_NOOUT,
    WOLFCLU_TEXT_OUT,
    WOLFCLU_SILENT,
    WOLFCLU_OUTPUT,
    WOLFCLU_PBKDF2,
    WOLFCLU_HELP,
    WOLFCLU_DEBUG,

};

/* algos */
/* hashing */
#define WOLFCLU_CERT_SHA    2000
#define WOLFCLU_CERT_SHA224 2001
#define WOLFCLU_CERT_SHA256 2002
#define WOLFCLU_CERT_SHA384 2003
#define WOLFCLU_CERT_SHA512 2004
#define WOLFCLU_MD5         2005

/* public key */
#define WOLFCLU_RSA     2006
#define WOLFCLU_ECC     2007
#define WOLFCLU_ED25519 2008

/* AES */
#define WOLFCLU_AES128CTR 2009
#define WOLFCLU_AES192CTR 2010
#define WOLFCLU_AES256CTR 2011
#define WOLFCLU_AES128CBC 2012
#define WOLFCLU_AES192CBC 2013
#define WOLFCLU_AES256CBC 2014

/* camellia */
#define WOLFCLU_CAMELLIA128CBC 2015
#define WOLFCLU_CAMELLIA192CBC 2016
#define WOLFCLU_CAMELLIA256CBC 2017

/* 3des */
#define WOLFCLU_DESCBC 2018


#define WOLFCLU_PBKDF2 2
#define WOLFCLU_PBKDF1 1

/* Structure for holding modes for long arguments. These are the modes that then
 * will take in any additional arguments. An example of a mode would be x509
 * in the command 'wolfssl x509 -in cert.pem -text' */
static struct option mode_options[] = {
    {"encrypt",   required_argument, 0, WOLFCLU_ENCRYPT   },
    {"decrypt",   required_argument, 0, WOLFCLU_DECRYPT   },
    {"enc",       no_argument,       0, WOLFCLU_CRYPT     },
    {"bench",     no_argument,       0, WOLFCLU_BENCHMARK },
    {"hash",      required_argument, 0, WOLFCLU_HASH      },
    {"md5",       no_argument,       0, WOLFCLU_MD5       },
    {"x509",      no_argument,       0, WOLFCLU_X509      },
    {"req",       no_argument,       0, WOLFCLU_REQUEST   },
    {"genkey",    required_argument, 0, WOLFCLU_GEN_KEY   },
    {"ecparam",   no_argument,       0, WOLFCLU_ECPARAM   },
    {"pkey",      no_argument,       0, WOLFCLU_PKEY      },
    {"rsa",       no_argument,       0, WOLFCLU_RSA       },
    {"ecc",       no_argument,       0, WOLFCLU_ECC       },
    {"ed25519",   no_argument,       0, WOLFCLU_ED25519   },
    {"dgst",      no_argument,       0, WOLFCLU_DGST      },
    {"help",      no_argument,       0, WOLFCLU_HELP      },
    {"h",         no_argument,       0, WOLFCLU_HELP      },
    {"v",         no_argument,       0, 'v'       },
    {"version",   no_argument,       0, 'v'       },

    {0, 0, 0, 0} /* terminal element */
};


/* Structure for holding long arguments for encrypt/decrypt */
static struct option crypt_options[] = {
    {"sha",       no_argument,       0, WOLFCLU_CERT_SHA   },
    {"sha224",    no_argument,       0, WOLFCLU_CERT_SHA224},
    {"sha256",    no_argument,       0, WOLFCLU_CERT_SHA256},
    {"sha384",    no_argument,       0, WOLFCLU_CERT_SHA384},
    {"sha512",    no_argument,       0, WOLFCLU_CERT_SHA512},

    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"pwd",       required_argument, 0, WOLFCLU_PASSWORD  },
    {"key",       required_argument, 0, WOLFCLU_KEY       },
    {"iv",        required_argument, 0, WOLFCLU_IV        },
    {"inkey",     required_argument, 0, WOLFCLU_INKEY     },
    {"output",    required_argument, 0, WOLFCLU_OUTPUT    },
    {"pbkdf2",    no_argument,       0, WOLFCLU_PBKDF2    },
    {"md",        required_argument, 0, WOLFCLU_MD        },
    {"d",         no_argument,       0, WOLFCLU_DECRYPT   },
    {"p",         no_argument,       0, WOLFCLU_DEBUG     },
    {"k",         required_argument, 0, WOLFCLU_PASSWORD  },

    {0, 0, 0, 0} /* terminal element */
};


static struct option pkey_options[] = {
    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"inform",    required_argument, 0, WOLFCLU_INFORM    },
    {"pubout",    no_argument,       0, WOLFCLU_PUBOUT    },
    {"help",      no_argument,       0, WOLFCLU_HELP      },
    {"h",         no_argument,       0, WOLFCLU_HELP      },

    {0, 0, 0, 0} /* terminal element */
};

static struct option ecparam_options[] = {
    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"outform",   required_argument, 0, WOLFCLU_OUTFORM   },
    {"genkey",    no_argument,       0, WOLFCLU_GEN_KEY    },
    {"name",      required_argument, 0, WOLFCLU_CURVE_NAME },

    {0, 0, 0, 0} /* terminal element */
};

static struct option crypt_algo_options[] = {
    /* AES */
    {"aes-128-ctr", no_argument, 0, WOLFCLU_AES128CTR},
    {"aes-192-ctr", no_argument, 0, WOLFCLU_AES192CTR},
    {"aes-256-ctr", no_argument, 0, WOLFCLU_AES256CTR},
    {"aes-128-cbc", no_argument, 0, WOLFCLU_AES128CBC},
    {"aes-192-cbc", no_argument, 0, WOLFCLU_AES192CBC},
    {"aes-256-cbc", no_argument, 0, WOLFCLU_AES256CBC},

    /* camellia */
    {"camellia-128-cbc", no_argument, 0, WOLFCLU_CAMELLIA128CBC},
    {"camellia-192-cbc", no_argument, 0, WOLFCLU_CAMELLIA192CBC},
    {"camellia-256-cbc", no_argument, 0, WOLFCLU_CAMELLIA256CBC},

    /* 3des */
    {"des-cbc", no_argument, 0, WOLFCLU_DESCBC},
    {"d",       no_argument, 0, WOLFCLU_DECRYPT},

    {0, 0, 0, 0} /* terminal element */
};


static struct option dgst_options[] = {

    {"sha",       no_argument,       0, WOLFCLU_CERT_SHA   },
    {"sha224",    no_argument,       0, WOLFCLU_CERT_SHA224},
    {"sha256",    no_argument,       0, WOLFCLU_CERT_SHA256},
    {"sha384",    no_argument,       0, WOLFCLU_CERT_SHA384},
    {"sha512",    no_argument,       0, WOLFCLU_CERT_SHA512},

    {"signature", required_argument, 0, WOLFCLU_INFILE    },
    {"verify",    required_argument, 0, WOLFCLU_VERIFY    },

    {0, 0, 0, 0} /* terminal element */
};

static struct option req_options[] = {

    {"sha",       no_argument,       0, WOLFCLU_CERT_SHA   },
    {"sha224",    no_argument,       0, WOLFCLU_CERT_SHA224},
    {"sha256",    no_argument,       0, WOLFCLU_CERT_SHA256},
    {"sha384",    no_argument,       0, WOLFCLU_CERT_SHA384},
    {"sha512",    no_argument,       0, WOLFCLU_CERT_SHA512},
    {"rsa",       no_argument,       0, WOLFCLU_RSA       },
    {"ecc",       no_argument,       0, WOLFCLU_ECC       },
    {"ed25519",   no_argument,       0, WOLFCLU_ED25519   },

    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"key",       required_argument, 0, WOLFCLU_KEY       },
    {"new",       no_argument,       0, WOLFCLU_NEW       },
    {"inkey",     required_argument, 0, WOLFCLU_INKEY     },
    {"inform",    required_argument, 0, WOLFCLU_INFORM    },
    {"outform",   required_argument, 0, WOLFCLU_OUTFORM   },
    {"config",    required_argument, 0, WOLFCLU_CONFIG },
    {"days",      required_argument, 0, WOLFCLU_DAYS },
    {"x509",      no_argument,       0, WOLFCLU_X509 },

    {0, 0, 0, 0} /* terminal element */
};


/* Structure for holding long arguments. These are arguments passed into the
 * modes. An example of an argument would be '-out' in the following command
 * 'wolfssl x509 -out cert.pem' */
static struct option long_options[] = {

    {"sha",       no_argument,       0, WOLFCLU_CERT_SHA   },
    {"sha224",    no_argument,       0, WOLFCLU_CERT_SHA224},
    {"sha256",    no_argument,       0, WOLFCLU_CERT_SHA256},
    {"sha384",    no_argument,       0, WOLFCLU_CERT_SHA384},
    {"sha512",    no_argument,       0, WOLFCLU_CERT_SHA512},

    {"signature", required_argument, 0, WOLFCLU_INFILE    },
    {"in",        required_argument, 0, WOLFCLU_INFILE    },
    {"out",       required_argument, 0, WOLFCLU_OUTFILE   },
    {"pwd",       required_argument, 0, WOLFCLU_PASSWORD  },
    {"key",       required_argument, 0, WOLFCLU_KEY       },
    {"new",       no_argument,       0, WOLFCLU_NEW       },
    {"iv",        required_argument, 0, WOLFCLU_IV        },
    {"all",       no_argument,       0, WOLFCLU_ALL       },
    {"size",      required_argument, 0, WOLFCLU_SIZE      },
    {"exponent",  required_argument, 0, WOLFCLU_EXPONENT  },
    {"time",      required_argument, 0, WOLFCLU_TIME      },
    {"sign",      no_argument,       0, WOLFCLU_SIGN      },
    {"verify",    no_argument,       0, WOLFCLU_VERIFY    },
    {"verbose",   no_argument,       0, WOLFCLU_VERBOSE   },
    {"inkey",     required_argument, 0, WOLFCLU_INKEY     },
    {"pubin",     no_argument,       0, WOLFCLU_PUBIN     },
    {"pubout",    no_argument,       0, WOLFCLU_PUBOUT    },
    {"pubkey",    no_argument,       0, WOLFCLU_PUBKEY    },
    {"inform",    required_argument, 0, WOLFCLU_INFORM    },
    {"outform",   required_argument, 0, WOLFCLU_OUTFORM   },
    {"sigfile",   required_argument, 0, WOLFCLU_SIGFILE   },
    {"config",    required_argument, 0, WOLFCLU_CONFIG    },
    {"days",      required_argument, 0, WOLFCLU_DAYS      },
    {"noout",     no_argument,       0, WOLFCLU_NOOUT     },
    {"text",      no_argument,       0, WOLFCLU_TEXT_OUT  },
    {"silent",    no_argument,       0, WOLFCLU_SILENT    },
    {"output",    required_argument, 0, WOLFCLU_OUTPUT    },
    {"pbkdf2",    no_argument,       0, WOLFCLU_PBKDF2    },
    {"k",         required_argument, 0, WOLFCLU_PASSWORD  },
    {"d",         no_argument,       0, WOLFCLU_DECRYPT   },

    {0, 0, 0, 0} /* terminal element */
};

/* method for converting arguments to lower case */
void convert_to_lower(char* s, int sSz);

#endif /* WOLFCLU_OPTARGS_H */

